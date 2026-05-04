<?php
require_once __DIR__ . '/../api/config.php';

header('Content-Type: application/json');

$token = $_GET['token'] ?? '';

if (!$token) {
    echo json_encode(['success' => false, 'message' => 'Invalid verification link']);
    exit;
}

$hashedToken = hash('sha256', $token);

// Find user
$stmt = $conn->prepare("
    SELECT id, verification_expires, is_verified 
    FROM users 
    WHERE verification_token = ?
");
$stmt->bind_param("s", $hashedToken);
$stmt->execute();

$result = $stmt->get_result();

if ($result->num_rows === 0) {
    echo json_encode(['success' => false, 'message' => 'Invalid token']);
    exit;
}

$user = $result->fetch_assoc();

// Already verified
if ($user['is_verified']) {
    echo json_encode(['success' => false, 'message' => 'Account already verified']);
    exit;
}

// Expiry check
if (!$user['verification_expires'] || strtotime($user['verification_expires']) < time()) {
    echo json_encode(['success' => false, 'message' => 'Verification link expired']);
    exit;
}

// Update user (single-use safe)
$stmt = $conn->prepare("
    UPDATE users 
    SET is_verified = 1
    WHERE id = ? AND is_verified = 0
");
$stmt->bind_param("i", $user['id']);
$stmt->execute();

if ($stmt->affected_rows > 0) {
    echo json_encode([
        'success' => true,
        'message' => 'Account verified successfully'
    ]);
} else {
    echo json_encode([
        'success' => false,
        'message' => 'Verification failed or already used'
    ]);
}

$stmt->close();
$conn->close();
