<?php
require __DIR__ . '/../vendor/autoload.php';

include_once 'config.php';
include_once 'utility.php';

use Firebase\JWT\JWT;

header('Content-Type: application/json');

// ✅ Only POST allowed
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    simpleResponse(['success' => false, 'message' => 'Invalid request method'], 405);
}

// ✅ Get refresh token from cookie
$refreshToken = $_COOKIE['refresh_token'] ?? '';

if (!$refreshToken) {
    simpleResponse(['success' => false, 'message' => 'No refresh token'], 401);
}

$refreshHash = hash('sha256', $refreshToken);

// ✅ Find token
$stmt = $conn->prepare("
    SELECT id, user_id, expires_at 
    FROM user_tokens 
    WHERE refresh_token_hash = ?
");
$stmt->bind_param("s", $refreshHash);

if (!$stmt->execute()) {
    simpleResponse(['success' => false, 'message' => 'Database error'], 500);
}

$result = $stmt->get_result();

if ($result->num_rows === 0) {
    // 🚨 Possible reuse attack (token already rotated or stolen)
    // OPTIONAL: revoke all tokens for this user if you can detect user
    simpleResponse(['success' => false, 'message' => 'Invalid token'], 401);
}

$tokenData = $result->fetch_assoc();

// ❌ Expired token
if (strtotime($tokenData['expires_at']) < time()) {

    // Cleanup this token
    $del = $conn->prepare("DELETE FROM user_tokens WHERE id = ?");
    $del->bind_param("i", $tokenData['id']);
    $del->execute();

    simpleResponse(['success' => false, 'message' => 'Expired token'], 401);
}


// =========================
// 🔐 ROTATION WITH TRANSACTION
// =========================
$conn->begin_transaction();

try {

    // 🔥 Delete old token (rotation)
    $delete = $conn->prepare("DELETE FROM user_tokens WHERE id = ?");
    $delete->bind_param("i", $tokenData['id']);

    if (!$delete->execute()) {
        throw new Exception("Delete failed");
    }

    // 🔄 Create new refresh token
    $newRefreshToken = bin2hex(random_bytes(32));
    $newHash = hash('sha256', $newRefreshToken);
    $newExpiry = date("Y-m-d H:i:s", time() + (86400 * 30)); // 30 days

    $insert = $conn->prepare("
        INSERT INTO user_tokens (user_id, refresh_token_hash, expires_at) 
        VALUES (?, ?, ?)
    ");
    $insert->bind_param("iss", $tokenData['user_id'], $newHash, $newExpiry);

    if (!$insert->execute()) {
        throw new Exception("Insert failed");
    }

    // 🧹 Cleanup expired tokens
    $conn->query("DELETE FROM user_tokens WHERE expires_at < NOW()");

    // ✅ Commit transaction
    $conn->commit();

} catch (Exception $e) {
    $conn->rollback();
    simpleResponse(['success' => false, 'message' => 'Server error'], 500);
}


// =========================
// 🔐 CREATE ACCESS TOKEN
// =========================
$jti = bin2hex(random_bytes(16));

$payload = [
    "iss" => $JWT_ISSUER,
    "iat" => time(),
    "nbf" => time(),
    "exp" => time() + 3600, // 1 hour
    "jti" => $jti,
    "data" => [
        "id" => $tokenData['user_id']
    ]
];

$accessToken = JWT::encode($payload, $JWT_SECRET, 'HS256');


// =========================
// 🍪 SET COOKIES
// =========================

// Access token
setcookie("auth_token", $accessToken, [
    'expires' => time() + 3600,
    'path' => '/',
    'secure' => true,
    'httponly' => true,
    'samesite' => 'Strict'
]);

// Refresh token (rotated)
setcookie("refresh_token", $newRefreshToken, [
    'expires' => time() + (86400 * 30),
    'path' => '/api/refresh.php',
    'secure' => true,
    'httponly' => true,
    'samesite' => 'Strict'
]);


// =========================
// ✅ RESPONSE
// =========================
simpleResponse([
    'success' => true,
    'message' => 'Token refreshed'
]);