<?php
// slider_write.php — accepts POST JSON: { node, r, g, b, LED1 }
// Updates /API/results.txt with RGB values and LED1 state, returns current state as JSON

$STATE_FILE = __DIR__ . '/results.txt';

function load_state($f){
  if(!file_exists($f)) return ['version'=>1,'nodes'=>[]];
  $j = json_decode(@file_get_contents($f), true);
  return $j ?: ['version'=>1,'nodes'=>[]];
}
function save_state($f,$s){
  $fp=fopen($f,'c+');
  if(!$fp){ http_response_code(500); exit('Cannot open file'); }
  if(flock($fp,LOCK_EX)){
    ftruncate($fp,0);
    fwrite($fp,json_encode($s,JSON_PRETTY_PRINT));
    fflush($fp);
    flock($fp,LOCK_UN);
  }
  fclose($fp);
}

header('Content-Type: application/json');
header('Cache-Control: no-store');

$method = $_SERVER['REQUEST_METHOD'];
if ($method !== 'POST' && $method !== 'GET') {
  http_response_code(405);
  echo json_encode(['ok'=>false,'error'=>'Method not allowed']);
  exit;
}

$state = load_state($STATE_FILE);
$node  = 'Node1';
$r = $g = $b = 0;
$led = 'off';

// Support both POST JSON and legacy GET (for quick manual tests)
if ($method === 'POST') {
  $raw = file_get_contents('php://input');
  $j = json_decode($raw, true);
  if (!$j) { http_response_code(400); echo json_encode(['ok'=>false,'error'=>'Invalid JSON']); exit; }

  $node = $j['node'] ?? 'Node1';
  $r = max(0, min(255, intval($j['r'] ?? 0)));
  $g = max(0, min(255, intval($j['g'] ?? 0)));
  $b = max(0, min(255, intval($j['b'] ?? 0)));
  $led = strtolower($j['LED1'] ?? 'off');
} else { // GET fallback: /slider_write.php?r=100&g=150&b=200&led=on
  $node = $_GET['node'] ?? 'Node1';
  $r = max(0, min(255, intval($_GET['r'] ?? 0)));
  $g = max(0, min(255, intval($_GET['g'] ?? 0)));
  $b = max(0, min(255, intval($_GET['b'] ?? 0)));
  $led = strtolower($_GET['led'] ?? 'off');
}

if (!isset($state['nodes'][$node])) {
  $state['nodes'][$node] = ['LED1'=>'off', 'RGB'=>['r'=>0,'g'=>0,'b'=>0]];
}

// Update values
$state['nodes'][$node]['RGB']['r'] = $r;
$state['nodes'][$node]['RGB']['g'] = $g;
$state['nodes'][$node]['RGB']['b'] = $b;
$state['nodes'][$node]['LED1']     = ($led === 'on') ? 'on' : 'off';

// Timestamp
$state['nodes'][$node]['updated_at'] = gmdate('c');

// Save
save_state($STATE_FILE, $state);

// Respond with whole state (so the page can refresh its state panel)
echo json_encode(['ok'=>true, 'state'=>$state]);
