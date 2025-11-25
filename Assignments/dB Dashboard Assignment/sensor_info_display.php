<?php
// --- DATABASE CONNECTION --- //
$servername = "127.0.0.1:3306";      // database host server IP address
$username   = "(change to actual username)";  // MySQL username
$password   = "(change to actual password)";          // MySQL password
$dbname     = "(change to actual db name)";    // database name

$conn = new mysqli($servername, $username, $password, $dbname); // connect to MySQL

// Check connection
if ($conn->connect_error) {
    die("<h3 style='color:red;'>Connection failed: " . $conn->connect_error . "</h3>");
}
?>
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>SSU IoT Lab</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { font-family: Arial, sans-serif; background-color: #f9fcff; text-align: center; } --Page Display Adjustments to make things look nicer
        h1 { color: #333; }
        table { margin: 20px auto; border-collapse: collapse; width: 70%; }
        th, td { border: 1px solid #999; padding: 10px; text-align: center; }
        th { background-color: #a4d792; }
        tr:nth-child(even) { background-color: #f2f2f2; }
        caption { font-weight: bold; margin-bottom: 10px; }
        .section { width: 80%; margin: 20px auto; text-align: left; }
        pre { background:#fff; border:1px solid #ddd; padding:10px; overflow:auto; }
        canvas { max-width: 900px; width: 95%; height: 360px; }
    </style>
</head>
<body>

    <h1>Welcome to SSU IoT Lab</h1>

    <!-- REGISTERED SENSOR NODES -->
    <table>
        <caption>Registered Sensor Nodes</caption>
        <tr>
            <th>Node Name</th>
            <th>Manufacturer</th>
        </tr>
        <?php
        $sql1 = "SELECT * FROM sensor_register ORDER BY node_name";
        $result1 = $conn->query($sql1);

        if ($result1 && $result1->num_rows > 0) {
            while ($row = $result1->fetch_assoc()) {
                echo "<tr>
                        <td>{$row['node_name']}</td>
                        <td>{$row['manufacturer']}</td>
                      </tr>";
            }
        } else {
            echo "<tr><td colspan='2'>No data found</td></tr>";
        }
        ?>
    </table>

    <!-- SENSOR DATA TABLE -->
    <table>
        <caption>Data Received</caption>
        <tr>
            <th>Node Name</th>
            <th>Time</th>
            <th>Temperature</th>
            <th>Humidity</th>
        </tr>
        <?php
        $sql2 = "SELECT * FROM sensor_data ORDER BY node_name, time_received";
        $result2 = $conn->query($sql2);

        if ($result2 && $result2->num_rows > 0) {
            while ($row = $result2->fetch_assoc()) {
                echo "<tr>
                        <td>{$row['node_name']}</td>
                        <td>{$row['time_received']}</td>
                        <td>{$row['temperature']}</td>
                        <td>{$row['humidity']}</td>
                      </tr>";
            }
        } else {
            echo "<tr><td colspan='4'>No data found</td></tr>";
        }
        ?>
    </table>

    <?php
    // --- Averages for node_1 ---
    $avg = $conn->query("SELECT AVG(temperature) AS avgT, AVG(humidity) AS avgH 
                         FROM sensor_data WHERE node_name='node_1'")->fetch_assoc();
    echo "<p><b>The Average Temperature for Node 1 has been:</b> " . round($avg['avgT'] ?? 0, 2) . "</p>";
    echo "<p><b>The Average Humidity for Node 1 has been:</b> " . round($avg['avgH'] ?? 0, 2) . "</p>";

    // --- Fetch time/temperature for Chart.js (node_1) ---
    $qChart = "SELECT time_received, temperature 
               FROM sensor_data 
               WHERE node_name='node_1' 
               ORDER BY time_received";
    $rChart = $conn->query($qChart);

    $labels = [];
    $temps  = [];
    if ($rChart) {
        while ($row = $rChart->fetch_assoc()) {
            $labels[] = $row['time_received'];
            $temps[]  = (float)$row['temperature'];
        }
    }

    // Build JSON for JS and the “Show one node’s data in JSON” requirement
    $nodeJson = [
        "node_name"      => "node_1",
        "time_received"  => $labels,
        "temperature"    => $temps
    ];

    // Prepare JS-safe JSON strings
    $labels_js = json_encode($labels);
    $temps_js  = json_encode($temps);
    $node_json_pretty = json_encode($nodeJson, JSON_PRETTY_PRINT);
    ?>

    <!-- CHART: Temperature vs Time for node_1 -->
    <div class="section">
        <h3>Temperature Data for Sensor Node 1</h3>
        <canvas id="tempChart"></canvas>
    </div>

    <!-- JSON view for assignment Q3  
    <div class="section">
        <h3>Node 1 Data (JSON)</h3>
        <pre><?php echo htmlspecialchars($node_json_pretty, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8'); ?></pre>
    </div>
    -->

    <script>
    // === Chart.js config ===
    const ctx = document.getElementById('tempChart').getContext('2d');
    const tempChart = new Chart(ctx, {
        // To change chart type: change 'bar' to 'line' to turn this into a line graph.
        type: 'bar',
        data: {
            labels: <?php echo $labels_js; ?>,      // X-axis (time_received)
            datasets: [{
                label: 'Sensor Node 1',
                data: <?php echo $temps_js; ?>,     // Y-axis (temperature)
                // These values change color outputs of the graph
                backgroundColor: 'rgba(0, 200, 0, 0.4)',
                borderColor: 'rgba(0, 150, 0, 1)',
                borderWidth: 1,
                tension: 0.3           // used when type is chosen to be 'line'
            }]
        },
        options: {
            responsive: true,
            plugins: {
                title: { display: true, text: 'Sensor Node 1 Temperature Data' },
                legend: { display: true }
            },
            scales: {
                x: { title: { display: true, text: 'Time' } },
                y: { title: { display: true, text: 'Temperature (°F)' }, beginAtZero: false }
            }
        }
    });

    </script>

<?php $conn->close(); ?>
</body>
</html>
