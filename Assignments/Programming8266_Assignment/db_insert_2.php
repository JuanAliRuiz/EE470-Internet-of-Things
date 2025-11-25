<?php
/* STRICT INSERT ENDPOINT (ultrasonic + photoresistor)
 * Tables: SensorRegister, SensorData (trigger updates SensorActivity)
 * Required: node_name, time_received (YYYY-MM-DD HH:MM:SS), and >=1 sensor value
 * Optional: source = SWITCH | TILT
 */
 
 /*
 //For Debugging, Remove once the HTTP 500 Error is fixed
header('Content-Type: text/plain');
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);
mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);
*/


$mysqli = new mysqli("127.0.0.1", "(change to username)", "(change to password)", "(change to db name)", 3306);
if ($mysqli->connect_error) { http_response_code(500); die("DB connect error"); }

// Inputs (GET or POST)
$node = isset($_REQUEST['node_name'])     ? trim($_REQUEST['node_name'])     : null;
$time = isset($_REQUEST['time_received']) ? trim($_REQUEST['time_received']) : null;

$temp = isset($_REQUEST['temp'])        ? $_REQUEST['temp']        : null;   // not used for your plan, but allowed
$hum  = isset($_REQUEST['humidity'])    ? $_REQUEST['humidity']    : null;   // not used for your plan, but allowed
$light= isset($_REQUEST['light'])       ? $_REQUEST['light']       : null;   // photoresistor (Node_2)
$prox = isset($_REQUEST['proximity'])   ? $_REQUEST['proximity']   : null;   // ultrasonic distance (Node_1, cm)
$src  = isset($_REQUEST['source'])      ? strtoupper(trim($_REQUEST['source'])) : null; // SWITCH | TILT

// Basic validation
if (!$node || !$time) { http_response_code(400); die("missing node_name or time_received"); }
if (!preg_match('/^\d{4}-\d{2}-\d{2}\s\d{2}:\d{2}:\d{2}$/', $time)) {
  http_response_code(400); die("time_received must be YYYY-MM-DD HH:MM:SS");
}

// At least one sensor value must be present
if ($temp===null && $hum===null && $light===null && $prox===null) {
  http_response_code(400); die("no sensor values provided");
}

// Numeric checks only for provided values
foreach (['temp'=>$temp,'humidity'=>$hum,'light'=>$light,'proximity'=>$prox] as $k=>$v) {
  if ($v!==null && !is_numeric($v)) { http_response_code(400); die("bad ".$k); }
}
if ($src && !in_array($src, ['SWITCH','TILT'])) { http_response_code(400); die("bad source"); }

// Ensure node is registered
$chk = $mysqli->prepare("SELECT 1 FROM SensorRegister WHERE node_name=?");
$chk->bind_param("s", $node);
$chk->execute(); $chk->store_result();
if ($chk->num_rows===0) { $chk->close(); http_response_code(403); die("unregistered node"); }
$chk->close();

// Insert (any nulls are fine)
$stmt = $mysqli->prepare(
  "INSERT INTO SensorData (node_name, time_received, temp, humidity, light, proximity, source)
   VALUES (?, ?, ?, ?, ?, ?, ?)"
);
$stmt->bind_param("ssdddds", $node, $time, $temp, $hum, $light, $prox, $src); // note: space OK; or use "ssdddds"
if (!$stmt->execute()) {
  if ($mysqli->errno==1062) { http_response_code(409); echo "Duplicate data rejected"; }
  else { http_response_code(500); echo "Insert failed"; }
} else {
  echo "OK";
}
$stmt->close(); $mysqli->close();
?>
