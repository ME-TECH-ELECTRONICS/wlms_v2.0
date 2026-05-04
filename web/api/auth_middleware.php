<?php
require_once __DIR__ . '/../vendor/autoload.php';
require_once __DIR__ . '/../api/config.php';

use Firebase\JWT\JWT;
use Firebase\JWT\Key;

function authenticate()
{
    global $conn, $JWT_SECRET, $JWT_ISSUER;

    // ✅ Get token
    $jwt = $_COOKIE['auth_token'] ?? '';

    if (!$jwt) {
        return ['success' => false, 'code' => 401, 'message' => 'Unauthorized'];
    }

    // =========================
    // 🔐 Verify JWT FIRST
    // =========================
    try {
        $decoded = JWT::decode($jwt, new Key($JWT_SECRET, 'HS256'));
    } catch (Exception $e) {
        return ["success" => false, "code" => 401, "message" => "Invalid or expired token"];
    }

    if (!isset($decoded->iss) || $decoded->iss !== $JWT_ISSUER) {
        return ["success" => false, "code" => 401, "message" => "Invalid token issuer"];
    }

    // =========================
    // 🔥 Blacklist check (AFTER decode)
    // =========================
    if (!isset($decoded->jti)) {
        return ["success" => false, "code" => 401, "message" => "Invalid token"];
    }

    $jtiHash = hash('sha256', $decoded->jti);

    $stmt = $conn->prepare("SELECT id FROM jwt_blacklist WHERE token_hash = ?");
    $stmt->bind_param("s", $jtiHash);

    if (!$stmt->execute()) {
        return ["success" => false, "code" => 500, "message" => "Database error"];
    }

    if ($stmt->get_result()->num_rows > 0) {
        return ["success" => false, "code" => 401, "message" => "Session expired"];
    }

    if (!isset($decoded->data->id)) {
        return ["success" => false, "code" => 401, "message" => "Invalid token payload"];
    }

    // ✅ Return user data
    return [
        "success" => true,
        "user" => [
            "id" => $decoded->data->id,
            "email" => $decoded->data->email ?? null
        ]
    ];
}
