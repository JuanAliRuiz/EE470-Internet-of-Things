<?php
// ---------- DB CONNECT ----------
$servername = "127.0.0.1";
$username   = "(change to username)";
$password   = "(change to password)";
$dbname     = "(change to db name)";
$port       = 3306;

$conn = new mysqli($servername, $username, $password, $dbname, $port);
if ($conn->connect_error) {
  die("<h3 style='color:red;'>DB connect failed: ".$conn->connect_error."</h3>");
}

// ---------- INPUT ----------
$selected = isset($_GET['node']) ? trim($_GET['node']) : 'ALL';

// ---------- REGISTERED NODES + COUNTS ----------
$nodes = [];
$res = $conn->query("SELECT node_name, manufacturer FROM SensorRegister ORDER BY node_name");
if ($res) { while ($r=$res->fetch_assoc()) $nodes[]=$r; $res->free(); }

$counts = [];
$res = $conn->query("SELECT node_name, msg_count FROM SensorActivity");
if ($res) { while ($r=$res->fetch_assoc()) $counts[$r['node_name']]=(int)$r['msg_count']; $res->free(); }

// ---------- DATA (OPTIONALLY FILTERED) ----------
$data = [];
if ($selected === 'ALL') {
  $sql = "SELECT node_name, time_received, temp, humidity, light, proximity, source
          FROM SensorData ORDER BY node_name, time_received";
  $res = $conn->query($sql);
} else {
  $stmt = $conn->prepare(
    "SELECT node_name, time_received, temp, humidity, light, proximity, source
     FROM SensorData WHERE node_name=? ORDER BY time_received"
  );
  $stmt->bind_param("s", $selected);
  $stmt->execute();
  $res = $stmt->get_result();
}
if ($res) { while ($r=$res->fetch_assoc()) $data[]=$r; $res->free(); }
if (isset($stmt) && $stmt) $stmt->close();

// ---------- PICK METRIC & AVERAGE ----------
$metric = 'proximity';  // default
if ($selected !== 'ALL') {
  // If your naming is fixed (Node_1 = proximity, Node_2 = light), this is the simplest:
  if ($selected === 'Node_2') $metric = 'light';
  // If you prefer auto-detect, uncomment this block:
/*
  $stmt = $conn->prepare(
    "SELECT 
       SUM(CASE WHEN proximity IS NOT NULL THEN 1 ELSE 0 END) AS pcount,
       SUM(CASE WHEN light     IS NOT NULL THEN 1 ELSE 0 END) AS lcount
     FROM SensorData WHERE node_name=?"
  );
  $stmt->bind_param("s", $selected);
  $stmt->execute();
  $res = $stmt->get_result();
  if ($res && $row=$res->fetch_assoc()) $metric = ((int)$row['pcount'] >= (int)$row['lcount']) ? 'proximity' : 'light';
  if ($res) $res->free();
  $stmt->close();
*/
}

// Average for selected node
$avgValue = null; $avgLabel = '';
if ($selected !== 'ALL') {
  $stmt = $conn->prepare("SELECT AVG($metric) AS av FROM SensorData WHERE node_name=?");
  $stmt->bind_param("s", $selected);
  $stmt->execute();
  $res = $stmt->get_result();
  if ($res && $row=$res->fetch_assoc()) {
    $avgValue = ($row['av'] !== null) ? round((float)$row['av'], 2) : null;
    $avgLabel = ($metric==='proximity') ? "Average Proximity (cm)" : "Average Light (ADC)";
  }
  if ($res) $res->free();
  $stmt->close();
}

$conn->close();

// ---------- PREP CHART ARRAYS ----------
$labels=[]; $values=[];
if ($selected !== 'ALL') {
  foreach ($data as $row) {
    if ($row['node_name'] !== $selected) continue;
    if ($row[$metric] !== null) {
      $labels[] = $row['time_received'];
      $values[] = (float)$row[$metric];
    }
  }
}
?>
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>SSU IoT Lab — Display 2</title>
  <meta http-equiv="refresh" content="10">
  <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.1"></script>
  <style>
    body{font-family:Arial, sans-serif;background:#f9fcff;margin:0;padding:0 12px 40px;}
    h1{color:#333;text-align:center;margin:20px 0 10px;}
    .row{display:flex;flex-wrap:wrap;gap:16px;justify-content:center;}
    .card{background:#fff;border:1px solid #ddd;border-radius:10px;padding:16px;box-shadow:0 1px 3px rgba(0,0,0,.06);}
    table{border-collapse:collapse;width:100%;}
    th,td{border:1px solid #aaa;padding:8px;text-align:center;}
    th{background:#a4d792;}
    tr:nth-child(even){background:#f5f5f5;}
    .toolbar{text-align:center;margin:8px 0 18px;}
    .small{color:#666;font-size:13px;text-align:center;}
    .wide{width:1080px;max-width:100%;}
    .stat{text-align:center;margin:10px 0;font-weight:bold;}
  </style>
</head>
<body>

<h1>Welcome to SSU IoT Lab</h1>

<div class="toolbar">
  <form method="get">
    <label for="node">Show data for: </label>
    <select id="node" name="node" onchange="this.form.submit()">
      <option value="ALL" <?= $selected==='ALL'?'selected':''; ?>>All Nodes</option>
      <?php foreach ($nodes as $n): $nm=htmlspecialchars($n['node_name']); ?>
        <option value="<?= $nm ?>" <?= $selected===$nm?'selected':''; ?>><?= $nm ?></option>
      <?php endforeach; ?>
    </select>
    <noscript><button type="submit">Go</button></noscript>
  </form>
  <div class="small">Auto-refresh every 10s</div>
</div>

<div class="row">
  <div class="card wide">
    <h3 style="text-align:center;margin:0 0 6px;">Registered Sensor Nodes</h3>
    <table>
      <tr><th>Node Name</th><th>Manufacturer</th><th>Count (messages)</th></tr>
      <?php if ($nodes): foreach($nodes as $n): ?>
        <tr>
          <td><?= htmlspecialchars($n['node_name']) ?></td>
          <td><?= htmlspecialchars($n['manufacturer']) ?></td>
          <td><?= isset($counts[$n['node_name']]) ? (int)$counts[$n['node_name']] : 0 ?></td>
        </tr>
      <?php endforeach; else: ?>
        <tr><td colspan="3">No registered nodes</td></tr>
      <?php endif; ?>
    </table>
  </div>
</div>

<div class="row">
  <div class="card wide">
    <h3 style="text-align:center;margin:0 0 6px;">Data Received <?= $selected==='ALL'?'':'— '.htmlspecialchars($selected) ?></h3>
    <table>
      <tr>
        <th>Node</th><th>Time</th><th>Temp</th><th>Humidity</th>
        <th>Light</th><th>Proximity</th><th>Source</th>
      </tr>
      <?php if ($data): foreach($data as $r): ?>
        <tr>
          <td><?= htmlspecialchars($r['node_name']) ?></td>
          <td><?= htmlspecialchars($r['time_received']) ?></td>
          <td><?= $r['temp']!==null ? htmlspecialchars($r['temp']) : '' ?></td>
          <td><?= $r['humidity']!==null ? htmlspecialchars($r['humidity']) : '' ?></td>
          <td><?= $r['light']!==null ? htmlspecialchars($r['light']) : '' ?></td>
          <td><?= $r['proximity']!==null ? htmlspecialchars($r['proximity']) : '' ?></td>
          <td><?= htmlspecialchars($r['source'] ?? '') ?></td>
        </tr>
      <?php endforeach; else: ?>
        <tr><td colspan="7">No data found</td></tr>
      <?php endif; ?>
    </table>

    <?php if ($selected !== 'ALL'): ?>
      <div class="stat">
        <?php if ($avgValue !== null): ?>
          <?= htmlspecialchars($avgLabel) ?> for <b><?= htmlspecialchars($selected) ?></b>:
          <?= htmlspecialchars($avgValue) ?>
        <?php else: ?>
          No values yet for <?= htmlspecialchars($metric) ?> on <b><?= htmlspecialchars($selected) ?></b>.
        <?php endif; ?>
      </div>
    <?php endif; ?>
  </div>
</div>

<?php if ($selected !== 'ALL'): ?>
<div class="row">
  <div class="card wide">
    <h3 style="text-align:center;margin:0 0 6px;">
      <?= ($metric==='proximity' ? 'Proximity (cm)' : 'Light (ADC)') ?> over Time — <?= htmlspecialchars($selected) ?>
    </h3>
    <canvas id="chart" height="120"></canvas>
  </div>
</div>

<script>
const labels = <?= json_encode($labels) ?>;
const values = <?= json_encode($values) ?>;

const ctx = document.getElementById('chart').getContext('2d');
new Chart(ctx, {
  type: 'line',
  data: {
    labels,
    datasets: [{
      label: '<?= $metric==='proximity' ? 'Proximity (cm)' : 'Light (ADC)' ?>',
      data: values,
      borderWidth: 2,
      fill: false,
      tension: 0.15
    }]
  },
  options: {
    scales: {
      x: { title: { display:true, text:'Time' } },
      y: { title: { display:true, text:'<?= $metric==='proximity' ? 'cm' : 'ADC' ?>' } }
    }
  }
});
</script>
<?php endif; ?>

</body>
</html>
