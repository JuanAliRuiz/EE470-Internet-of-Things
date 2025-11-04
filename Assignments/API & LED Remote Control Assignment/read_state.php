<?php
$STATE_FILE = __DIR__ . '/results.txt';
header('Content-Type: application/json');
header('Cache-Control: no-store');
if(!file_exists($STATE_FILE)){ echo json_encode(["version"=>1,"nodes"=>[]]); exit; }
readfile($STATE_FILE);
