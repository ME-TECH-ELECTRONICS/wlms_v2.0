<?php
require_once __DIR__ . '/../api/config.php';
require_once __DIR__ . '/../api/utility.php';
require_once __DIR__ . '/../api/auth_middleware.php';

header('Content-Type: application/json');

if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    $input = json_decode(file_get_contents("php://input"), true);

    if (!$input) {
        simpleResponse(['success' => false, 'message' => 'Invalid JSON'], 400);
    }

    $deviceId = strtolower(trim($input['deviceId'] ?? ''));
    if (!preg_match('/^[a-f0-9]{12}$/', $deviceId)) {
        simpleResponse(['success' => false, 'message' => 'Invalid device ID'], 400);
    }
    $auth = authenticate();
    if (!$auth['success']) {
        http_response_code($auth['code']);
        echo json_encode($auth);
        exit;
    }
    $user_id = (int)$auth['user']['id'];
    $stmt = $conn->prepare("INSERT INTO user_devices (user_id, device_id) VALUES (?, ?) ON DUPLICATE KEY UPDATE device_id = VALUES(device_id)");
    $stmt->bind_param("ss", $user_id, $deviceId);

    if ($stmt->execute()) {
        simpleResponse(['success' => true, 'message' => 'Device added successfully']);
    } else {
        simpleResponse(['success' => false, 'message' => 'Failed to add device'], 500);
    }
    
} else {
    simpleResponse(['success' => false, 'message' => 'Invalid request method.'], 405);
}
