<?php
require_once __DIR__ . "/../api/config.php";
require_once __DIR__ . "/../api/utility.php";
require_once __DIR__ . "/../api/auth_middleware.php";


if ($_SERVER['REQUEST_METHOD'] != "GET") {
    simpleResponse(['success' => false, 'message' => 'Invalid request method']);
}
$device_id = $_GET['id'];
if (!isset($device_id)) {
    simpleResponse(["success" => false, "message" => "Missing device id"]);
}
if (!preg_match('/^[a-f0-9]{12}$/', $device_id)) {
    simpleResponse(['success' => false, 'message' => 'Invalid device ID'], 400);
}
$auth = authenticate();
if (!$auth['success']) {
    simpleResponse(['success' => false, 'message' => $auth['message']], $auth['code']);
}

$stmt = $conn->prepare("SELECT * FROM events WHERE device_id = ? ORDER BY created_on DESC LIMIT 1000");
$stmt->bind_param("s", $device_id);
if (!$stmt->execute()) {
    simpleResponse(['success' => false, 'message' => 'Database error']);
}
$result = $stmt->get_result();
$logs = [];
while ($row = $result->fetch_assoc()) {
    $logs[] = [
        "id" => $row['id'],
        "event_type" => $row['event'],
        "timestamp" => $row['created_on']
    ];
}
simpleResponse([
    "success" => true,
    "logs" => $logs
]);
