<?php
// --- DB CONFIG ---
$host     = "195.35.61.68";         
$user     = "databaseusername(replace with actual name)";
$password = "databasepasword(replace with actual password)";
$dbname   = "dbname(replace with actual name)";

// --- CONNECT ---
$conn = new mysqli($host, $user, $password, $dbname);
if ($conn->connect_error) {
    die("DB connection failed: " . $conn->connect_error);
}

// Get the last 50 samples
$sql = "SELECT value, ts FROM sensor_value ORDER BY ts DESC LIMIT 50";
$result = $conn->query($sql);

$values = [];
$timestamps = [];

if ($result && $result->num_rows > 0) {
    // Reverse order so oldest is first
    $rows = [];
    while ($row = $result->fetch_assoc()) {
        $rows[] = $row;
    }
    $rows = array_reverse($rows);

    foreach ($rows as $row) {
        $values[] = (float)$row['value'];
        $timestamps[] = $row['ts'];
    }
}
$conn->close();
?>
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>MQTT Sensor Values</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { font-family: Arial, sans-serif; margin: 30px; }
        h1 { font-size: 24px; margin-bottom: 10px; }
        #chart-container { width: 90%; max-width: 800px; }
    </style>
</head>
<body>
    <h1>Potentiometer Values from MQTT (sensor_value)</h1>
    <p>This plot shows the latest samples stored in the Hostinger database.</p>

    <div id="chart-container">
        <canvas id="sensorChart"></canvas>
    </div>

    <script>
        const labels = <?php echo json_encode($timestamps); ?>;
        const dataValues = <?php echo json_encode($values); ?>;

        const ctx = document.getElementById('sensorChart').getContext('2d');
        const sensorChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [{
                    label: 'Potentiometer Value',
                    data: dataValues,
                    fill: false,
                    borderWidth: 2,
                    tension: 0.1
                }]
            },
            options: {
                scales: {
                    x: { title: { display: true, text: 'Timestamp' } },
                    y: { title: { display: true, text: 'Value' } }
                }
            }
        });
    </script>
</body>
</html>
