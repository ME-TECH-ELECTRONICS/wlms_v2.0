<?php

require_once __DIR__ . '/../api/auth_middleware.php';
require_once __DIR__ . '/../api/utility.php';

header('Content-Type: application/json');

$auth = authenticate();
if (!$auth['success']) {
    simpleResponse([ 'success' => false, 'message' => $auth['message'] ], $auth['code']);
}
simpleResponse([ 'success' => true, 'message' => 'Authenticated']);
