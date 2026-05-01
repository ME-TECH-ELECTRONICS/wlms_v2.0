<?php

if($_SERVER['REQUEST_METHOD'] == 'POST') {
    $device_id = $_POST['deviceId'];

    // Validate input and check the length is 12
    if(empty($device_id) || strlen($device_id) != 12) {
        echo json_encode(['status' => 'error', 'message' => 'Device ID is required and must be 12 characters long.']);
        exit;
    }


    // Add device to the database (this is a placeholder, replace with actual database code)
    // Example: $db->query("INSERT INTO devices (name, type, location) VALUES ('$device_name', '$device_type', '$device_location')");

    echo json_encode(['status' => 'success', 'message' => 'Device added successfully.']);
} else {
    echo json_encode(['status' => 'error', 'message' => 'Invalid request method.']);
}