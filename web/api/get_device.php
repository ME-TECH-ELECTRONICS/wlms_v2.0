<?php
require_once __DIR__ . '/../vendor/autoload.php';

include_once __DIR__ . '/../api/config.php';
include_once __DIR__ . '/../api/utility.php';
include_once __DIR__ . '/../api/auth_middleware.php';

header('Content-Type: application/json');

// =========================
// ✅ ONLY ALLOW GET
// =========================

if ($_SERVER['REQUEST_METHOD'] !== 'GET') {
    simpleResponse([
        'success' => false,
        'message' => 'Invalid request method'
    ], 405);
}

// =========================
// 🔐 AUTHENTICATE USER
// =========================

$auth = authenticate();

if (!$auth['success']) {
    simpleResponse([
        'success' => false,
        'message' => $auth['message']
    ], $auth['code']);
}

$user = $auth['user'];

// =========================
// 📦 GET USER DEVICE
// =========================

$stmt = $conn->prepare("
    SELECT device_id 
    FROM user_devices 
    WHERE user_id = ?
    LIMIT 1
");

$stmt->bind_param("i", $user['id']);

if (!$stmt->execute()) {
    simpleResponse([
        'success' => false,
        'message' => 'Database error'
    ], 500);
}

$result = $stmt->get_result();

// =========================
// ❌ NO DEVICE FOUND
// =========================

if ($result->num_rows === 0) {
    simpleResponse([
        'success' => true,
        'device' => null
    ]);
}

$row = $result->fetch_assoc();

// =========================
// ✅ RESPONSE
// =========================

simpleResponse([
    'success' => true,
    'device' => $row['device_id']
]);
