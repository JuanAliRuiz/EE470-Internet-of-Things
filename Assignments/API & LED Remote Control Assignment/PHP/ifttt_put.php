<?php
// Accepts PUT JSON {"node":"Node1","event":"switch_pressed"}
// Then POSTs to IFTTT Webhooks to deliver to Slack (or SMS)

$IFTTT_KEY   = 'dd_l7cAk_rSgHTVPnUmKeW';
$IFTTT_EVENT = 'esp_event';

if ($_SERVER['REQUEST_METHOD'] !== 'PUT') { http_response_code(405); exit('Use PUT'); }
$raw = file_get_contents('php://input');
$j = json_decode($raw, true);
if (!$j) { http_response_code(400); exit('Invalid JSON'); }

$payload = [
  'value1' => $j['node']  ?? 'Node1',
  'value2' => $j['event'] ?? 'unknown',
  'value3' => gmdate('c'),
];

$ch = curl_init("https://maker.ifttt.com/trigger/{$IFTTT_EVENT}/with/key/{$IFTTT_KEY}");
curl_setopt_array($ch, [
  CURLOPT_POST => true,
  CURLOPT_RETURNTRANSFER => true,
  CURLOPT_HTTPHEADER => ['Content-Type: application/json'],
  CURLOPT_POSTFIELDS => json_encode($payload),
]);
$resp = curl_exec($ch);
$code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
curl_close($ch);

header('Content-Type: application/json');
echo json_encode(['ok' => ($code >= 200 && $code < 300), 'ifttt_code' => $code]);
