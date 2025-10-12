<?php
// --- DB CONNECT ---
$servername = "127.0.0.1";
$dbport     = 3306;
$username   = "u148378080_db_juanaliruiz";
$password   = "Jazmine03232018!";
$dbname     = "u148378080_juanaliruiz";

$conn = new mysqli($servername, $username, $password, $dbname, $dbport);
if ($conn->connect_error) { http_response_code(500); die("DB connection failed"); }

// --- INPUT HANDLING ---
// Accept either base64 "b64" or regular GET keys
$node = $temp = $hum = $time = null;

if (isset($_GET['b64'])) {
    // Important: Base64 must be URL-encoded; '+' should be %2B, '=' should be %3D
    $b64 = $_GET['b64'];
    $decoded = base64_decode($b64, true); // strict mode
    if ($decoded === false) { http_response_code(400); die("Invalid base64 payload"); }

    // Example decoded: nodeId=node_3&nodeTemp=34&nodeHum=40&timeReceived=2022-10-02 20:25:01
    parse_str($decoded, $p);
    $node = $p['nodeId']       ?? $p['node_name']      ?? null;
    $temp = $p['nodeTemp']     ?? $p['temperature']    ?? null;
    $hum  = $p['nodeHum']      ?? $p['humidity']       ?? null;
    $time = $p['timeReceived'] ?? $p['time_received']  ?? null;

} else {
    $node = $_GET['nodeId']        ?? $_GET['node_name']      ?? null;
    $temp = $_GET['nodeTemp']      ?? $_GET['temperature']    ?? null;
    $hum  = $_GET['nodeHum']       ?? $_GET['humidity']       ?? null;
    $time = $_GET['timeReceived']  ?? $_GET['time_received']  ?? null;
}

// --- VALIDATION ---
if (!$node || $temp === null || !$time) {
    http_response_code(400);
    die("Missing required fields: node, temperature, or time");
}
if (!is_numeric($temp)) { http_response_code(400); die("temperature must be numeric"); }
if ($hum === null || !is_numeric($hum)) { $hum = 0; } // default if not provided

// basic time format check
if (!preg_match('/^\d{4}-\d{2}-\d{2}\s\d{2}:\d{2}:\d{2}$/', $time)) {
    http_response_code(400); die("time must be 'YYYY-MM-DD HH:MM:SS'");
}

// optional range checks (tweak as needed)
if ($temp < -50 || $temp > 150) { http_response_code(422); die("temperature out of range"); }
if ($hum  <   0 || $hum  > 100) { http_response_code(422); die("humidity out of range"); }

// --- CHECK NODE REGISTERED (without get_result) ---
$chk = $conn->prepare("SELECT COUNT(*) FROM sensor_register WHERE node_name = ?");
$chk->bind_param("s", $node);
$chk->execute();
$chk->bind_result($count);
$chk->fetch();
$chk->close();
if ($count == 0) { http_response_code(403); die("Node not registered"); }

// --- INSERT ---
$ins = $conn->prepare(
    "INSERT INTO sensor_data (node_name, time_received, temperature, humidity) VALUES (?, ?, ?, ?)"
);
$ins->bind_param("ssdd", $node, $time, $temp, $hum);

if ($ins->execute()) {
    echo "OK! Decoded/parsed and inserted: node={$node}, temp={$temp}, hum={$hum}, time={$time}";
} else {
    http_response_code(500);
    echo "Insert failed: " . $ins->error; // temporary detail for debugging
}
$ins->close();
$conn->close();
