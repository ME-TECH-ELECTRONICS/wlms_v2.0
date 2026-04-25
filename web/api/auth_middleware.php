<?php
require_once __DIR__ . '/../vendor/autoload.php';

include_once 'config.php';
include_once 'db.php';

use Firebase\JWT\JWT;
use Firebase\JWT\Key;

function authenticate() {
    global $conn, $JWT_SECRET;

    header('Content-Type: application/json');

    // ✅ Get token
    $jwt = $_COOKIE['auth_token'] ?? '';

    if (!$jwt) {
        http_response_code(401);
        echo json_encode(['success' => false, 'message' => 'Unauthorized']);
        exit;
    }

    // =========================
    // 🔐 Verify JWT FIRST
    // =========================
    try {
        $decoded = JWT::decode($jwt, new Key($JWT_SECRET, 'HS256'));
    } catch (Exception $e) {
        http_response_code(401);
        echo json_encode(['success' => false, 'message' => 'Invalid or expired token']);
        exit;
    }

    // =========================
    // 🔥 Blacklist check (AFTER decode)
    // =========================
    if (!isset($decoded->jti)) {
        http_response_code(401);
        echo json_encode(['success' => false, 'message' => 'Invalid token']);
        exit;
    }

    $jtiHash = hash('sha256', $decoded->jti);

    $stmt = $conn->prepare("
        SELECT id FROM jwt_blacklist WHERE token_hash = ?
    ");
    $stmt->bind_param("s", $jtiHash);

    if (!$stmt->execute()) {
        http_response_code(500);
        echo json_encode(['success' => false, 'message' => 'Database error']);
        exit;
    }

    if ($stmt->get_result()->num_rows > 0) {
        http_response_code(401);
        echo json_encode(['success' => false, 'message' => 'Session expired']);
        exit;
    }

    // ✅ Return user data
    return $decoded->data;
}