<?php
require_once __DIR__ . "/../api/config.php";
require_once __DIR__ . "/../api/utility.php";
require_once __DIR__ . "/../api/auth_middleware.php";

if ($_SERVER['REQUEST_METHOD'] === "GET") {
    if (!isset($_GET['id'])) {
        simpleResponse(["success" => false, "message" => "Missing device id"]);
    }

    $device_id = $_GET['id'];
    $auth = authenticate();
    if (!$auth['success']) {
        http_response_code($auth['code']);
        echo json_encode($auth);
        exit;
    }

    $user_id = $auth['user']['id'];
    // echo "User ID: $user_id, Device ID: $device_id"; // Debugging line
    $stmt = $conn->prepare("SELECT * FROM user_devices WHERE device_id = ? AND user_id = ?");
    $stmt->bind_param("si", $device_id, $user_id);
    if (!$stmt->execute()) {
        simpleResponse(['success' => false, 'message' => 'Database error']);
    }
    $result = $stmt->get_result();

    if ($result->num_rows > 0) {
        $row = $result->fetch_assoc();

        simpleResponse([
            "success" => true,
            "data" => $row
        ]);
    } else {
        simpleResponse(["success" => false, "error" => "No record found"]);
    }
}

if ($_SERVER['REQUEST_METHOD'] === "POST") {
    $input = json_decode(file_get_contents("php://input"), true);
    if (!$input) {
        simpleResponse(['success' => false, 'message' => 'Invalid JSON'], 400);
    }

    $device_id = $input['deviceId'] ?? '';
    if (!preg_match('/^[a-f0-9]{12}$/', $device_id)) {
        simpleResponse(['success' => false, 'message' => 'Invalid device ID'], 400);
    }

    $auth = authenticate();
    if (!$auth['success']) {
        http_response_code($auth['code']);
        echo json_encode($auth);
        exit;
    }
    $user_id = $auth['user']['id'];
    if($input['motorToggle'] ?? false) {
        // Toggle motor state
        $stmt = $conn->prepare("SELECT motor, new_data FROM user_devices WHERE user_id = ? AND device_id = ?");
        $stmt->bind_param("is", $user_id, $device_id);
        if (!$stmt->execute()) {
            simpleResponse(['success' => false, 'message' => 'Database error']);
        }
        $result = $stmt->get_result();
        if ($result->num_rows > 0) {
            $row = $result->fetch_assoc();
            if($row['new_data'] == 1) {
                simpleResponse(['success' => false, 'message' => 'Previous command not yet processed'], 200);
            }
            $newState = $row['motor'] == 1 ? 0 : 1;
            $updateStmt = $conn->prepare("UPDATE user_devices SET motor = ?, new_data = 1 WHERE user_id = ? AND device_id = ?");
            $updateStmt->bind_param("iis", $newState, $user_id, $device_id);
            if ($updateStmt->execute()) {
                simpleResponse(['success' => true, 'message' => 'Motor state toggled']);
            } else {
                simpleResponse(['success' => false, 'message' => 'Failed to toggle motor state'], 500);
            }
        } else {
            simpleResponse(['success' => false, 'message' => 'Device not found'], 404);
        }
        exit;
    } else {
        if(!isset($input['startThreshold']) || !isset($input['stopThreshold']) || !isset($input['dryRunTimeout']) || !isset($input['dryRunCancel']) || !isset($input['motorTimeout']) || !isset($input['wifiSsid']) || !isset($input['wifiPassword'])) {
            simpleResponse(['success' => false, 'message' => 'Missing motor state'], 400);
        }

        //the field wifissid, wifipassword is string all otheres are int also wifissid only allows a-zA-Z0-9 and wifipassword allows a-zA-Z0-9_-
        if (!preg_match('/^[a-zA-Z0-9]{1,32}$/', $input['wifiSsid'])) {
            simpleResponse(['success' => false, 'message' => 'Invalid WiFi SSID'], 400);
        }
        if (!preg_match('/^[a-zA-Z0-9_\-@]{1,32}$/', $input['wifiPassword'])) {
            simpleResponse(['success' => false, 'message' => 'Invalid WiFi Password'], 400);
        }

        $stmt = $conn->prepare("UPDATE user_devices SET water_low_th = ?, water_high_th = ?, dry_run_timeout = ?, dry_run_cancel = ?, max_motor_runtime = ?, wifi_ssid = ?, wifi_password = ?, new_data = 1 WHERE user_id = ? AND device_id = ?");
        $stmt->bind_param("iiiisssis", $input['startThreshold'], $input['stopThreshold'], $input['dryRunTimeout'], $input['dryRunCancel'], $input['motorTimeout'], $input['wifiSsid'], $input['wifiPassword'], $user_id, $device_id);
        if ($stmt->execute()) {
            simpleResponse(['success' => true, 'message' => 'Settings updated']);
        } else {
            simpleResponse(['success' => false, 'message' => 'Failed to update settings'], 500);
        }
    }

}
