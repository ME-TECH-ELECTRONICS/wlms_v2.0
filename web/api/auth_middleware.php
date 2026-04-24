<?php
require __DIR__ . '/../vendor/autoload.php';

use Firebase\JWT\JWT;
use Firebase\JWT\Key;

include_once 'config.php';

function requireAuth() {
    global $JWT_SECRET;

    // 🍪 Get token from cookie
    if (!isset($_COOKIE['auth_token'])) {
        http_response_code(401);
        echo json_encode(['success' => false, 'message' => 'Unauthorized']);
        exit;
    }

    $token = $_COOKIE['auth_token'];

    try {
        // 🔐 Decode JWT
        $decoded = JWT::decode($token, new Key($JWT_SECRET, 'HS256'));

        // Optional: check issuer
        if ($decoded->iss !== $GLOBALS['JWT_ISSUER']) {
            throw new Exception("Invalid issuer");
        }

        // ✅ Return user data
        return $decoded->data;

    } catch (Exception $e) {
        http_response_code(401);
        echo json_encode([
            'success' => false,
            'message' => 'Invalid or expired token'
        ]);
        exit;
    }
}