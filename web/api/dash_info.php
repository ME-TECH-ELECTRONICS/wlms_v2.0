<?php
include "db.php";
include "utility.php";
include "auth_middleware.php";

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
