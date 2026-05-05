<?php
require_once __DIR__ . '/../api/config.php';
require_once __DIR__ . '/../api/utility.php';

header('Content-Type: application/json');

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    simpleResponse(['s' => 0, 'm' => 'Invalid method'], 405);
}

$input = json_decode(file_get_contents("php://input"), true);

if (!$input) {
    simpleResponse(['s' => 0, 'm' => 'Invalid JSON'], 400);
}

// Validate
if (!isset($input['id'], $input['wl'], $input['volt'], $input['mt'])) {
    simpleResponse(['s' => 0, 'm' => 'Missing parameters'], 400);
}

$device_id   = strtoupper((string)$input['id']);
$water_level = (int)$input['wl'];
$voltage     = (int)$input['volt'];
$motor_status = (bool)$input['mt'];

if ($water_level < 0 || $water_level > 100) {
    simpleResponse(['s' => 0, 'm' => 'Invalid water level'], 400);
}

// =====================
// UPDATE SENSOR DATA
// =====================
$update_stmt = $conn->prepare("
    UPDATE user_devices 
    SET level = ?, voltage = ?, motor = ?, last_updated = CURRENT_TIMESTAMP 
    WHERE device_id = ?
");

if (!$update_stmt) {
    simpleResponse(['s' => 0, 'm' => 'Prepare failed'], 500);
}

$update_stmt->bind_param("iiis", $water_level, $voltage, $motor_status, $device_id);

if (!$update_stmt->execute()) {
    simpleResponse(['s' => 0, 'm' => 'Update failed'], 500);
}

if ($update_stmt->affected_rows === 0) {
    simpleResponse(['s' => 0, 'm' => 'Device not found'], 404);
}

// =====================
// FETCH ONLY NEEDED FIELDS
// =====================
$select_stmt = $conn->prepare("
    SELECT 
        device_id,
        motor,
        water_low_th,
        water_high_th,
        dry_run_timeout,
        dry_run_cancel,
        max_motor_runtime,
        motor_start_day_hour,
        motor_stop_day_hour,
        wifi_ssid,
        wifi_password,
        new_data
    FROM user_devices 
    WHERE device_id = ?
");

$select_stmt->bind_param("s", $device_id);
$select_stmt->execute();

$row = $select_stmt->get_result()->fetch_assoc();

if (!$row) {
    simpleResponse(['s' => 0, 'm' => 'Device not found'], 404);
}

// =====================
// BUILD COMPACT RESPONSE
// =====================
$response = [
    's'  => 1,                         // success
    'nd' => (int)$row['new_data'],     // new_data flag
];

// Only send config if new_data = 1
if ((int)$row['new_data'] === 1) {

    $response['cfg'] = [
        'd'  => $row['device_id'],
        'm'  => (int)$row['motor'],
        'wl' => (int)$row['water_low_th'],
        'wh' => (int)$row['water_high_th'],
        'dt' => (int)$row['dry_run_timeout'],
        'dc' => (int)$row['dry_run_cancel'],
        'mr' => (int)$row['max_motor_runtime'],
        'sh' => (int)$row['motor_start_day_hour'],
        'eh' => (int)$row['motor_stop_day_hour'],
        'ws' => $row['wifi_ssid'],
        'wp' => $row['wifi_password']
    ];

    // 🔁 Reset flag AFTER sending
    $reset_stmt = $conn->prepare("
        UPDATE user_devices SET new_data = 0 WHERE device_id = ?
    ");
    $reset_stmt->bind_param("s", $device_id);
    $reset_stmt->execute();
}

// =====================
// RESPONSE
// =====================
simpleResponse($response);