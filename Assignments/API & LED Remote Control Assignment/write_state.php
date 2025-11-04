<?php
// Accepts:
//   GET:  ?node=Node1&led=LED1&value=on   (or off)
//   PUT:  {"node":"Node1","LED1":"on"}
// Writes JSON into /API/results.txt

$STATE_FILE = __DIR__ . '/results.txt';

function load_state($file){
  if(!file_exists($file)) return ["version"=>1,"nodes"=>[]];
  $txt = file_get_contents($file);
  $j = json_decode($txt, true);
  return $j ?: ["version"=>1,"nodes"=>[]];
}

function save_state($file, $state){
  $fp = fopen($file, 'c+');
  if(!$fp){ http_response_code(500); exit('Cannot open file'); }
  if(flock($fp, LOCK_EX)){
    ftruncate($fp, 0);
    fwrite($fp, json_encode($state, JSON_PRETTY_PRINT));
    fflush($fp);
    flock($fp, LOCK_UN);
  }
  fclose($fp);
}

$state = load_state($STATE_FILE);
$method = $_SERVER['REQUEST_METHOD'];

if($method === 'GET'){
  $node = $_GET['node'] ?? 'Node1';
  $led  = $_GET['led']  ?? 'LED1';
  $val  = $_GET['value']?? null; // on/off
  if($val===null){ http_response_code(400); exit('Missing value'); }

  $state['nodes'][$node] = $state['nodes'][$node] ?? ['LED1'=>'off'];
  $state['nodes'][$node][$led] = (strtolower($val)==='on') ? 'on' : 'off';
  $state['nodes'][$node]['updated_at'] = gmdate('c');
  save_state($STATE_FILE, $state);

  header('Content-Type: application/json');
  header('Cache-Control: no-store');
  echo json_encode(['ok'=>true,'state'=>$state]);
  exit;
}

if($method === 'PUT'){
  $raw = file_get_contents('php://input');
  $j = json_decode($raw, true);
  if(!$j){ http_response_code(400); exit('Invalid JSON'); }
  $node = $j['node'] ?? 'Node1';
  $val  = strtolower($j['LED1'] ?? 'off');

  $state['nodes'][$node] = $state['nodes'][$node] ?? ['LED1'=>'off'];
  $state['nodes'][$node]['LED1'] = ($val==='on') ? 'on' : 'off';
  $state['nodes'][$node]['updated_at'] = gmdate('c');
  save_state($STATE_FILE, $state);

  header('Content-Type: application/json');
  header('Cache-Control: no-store');
  echo json_encode(['ok'=>true,'state'=>$state]);
  exit;
}

http_response_code(405); echo 'Method not allowed';
