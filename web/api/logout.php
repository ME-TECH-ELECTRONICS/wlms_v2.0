<?php
require __DIR__ . '/../vendor/autoload.php';

include_once 'config.php';
include_once 'db.php';

use Firebase\JWT\JWT;
use Firebase\JWT\Key;

header('Content-Type: application/json');

// ✅ Helper
function respond($data, $code = 200)
{
    http_response_code($code);
    echo json_encode($data);
    exit;
}

// Only POST allowed
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    respond(['success' => false, 'message' => 'Invalid request method'], 405);
}

// Start transaction
$conn->begin_transaction();

try {

    // =========================
    // 🔥 DELETE REFRESH TOKEN (current session)
    // =========================
    $refreshToken = $_COOKIE['refresh_token'] ?? '';

    if (!empty($refreshToken)) {
        $refreshHash = hash('sha256', $refreshToken);

        $stmt = $conn->prepare("
            DELETE FROM user_tokens 
            WHERE refresh_token_hash = ?
        ");

        if (!$stmt) {
            throw new Exception("Prepare failed (refresh delete)");
        }

        $stmt->bind_param("s", $refreshHash);

        if (!$stmt->execute()) {
            throw new Exception("Failed to delete refresh token");
        }

        $stmt->close();
    }

    // =========================
    // 🔐 BLACKLIST ACCESS TOKEN (via jti)
    // =========================
    $jwt = $_COOKIE['auth_token'] ?? '';

    if (!empty($jwt)) {

        $parts = explode('.', $jwt);

        if (count($parts) === 3) {
            $payload = json_decode(base64_decode($parts[1]), true);

            $jti = $payload['jti'] ?? null;
            $exp = $payload['exp'] ?? null;

            if ($jti && $exp) {

                // Only blacklist if still valid
                if ($exp > time()) {

                    $jtiHash = hash('sha256', $jti);

                    // Use UTC-safe conversion
                    $expiresAt = gmdate("Y-m-d H:i:s", $exp);

                    $stmt = $conn->prepare("
                        INSERT INTO jwt_blacklist (token_hash, expires_at)
                        VALUES (?, ?)
                    ");

                    if (!$stmt) {
                        throw new Exception("Prepare failed (blacklist insert)");
                    }

                    $stmt->bind_param("ss", $jtiHash, $expiresAt);

                    if (!$stmt->execute()) {
                        throw new Exception("Failed to insert blacklist");
                    }

                    $stmt->close();
                }
            }
        }
    }

    // Commit all changes
    $conn->commit();

} catch (Exception $e) {

    $conn->rollback();

    // Do NOT leak internal errors
    respond([
        'success' => false,
        'message' => 'Logout failed'
    ], 500);
}


// =========================
// 🍪 CLEAR COOKIES
// =========================
function clearCookie($name, $path = '/')
{
    setcookie($name, '', [
        'expires' => time() - 3600,
        'path' => $path,
        'secure' => true,
        'httponly' => true,
        'samesite' => 'Strict'
    ]);
}

// Clear access token
clearCookie('auth_token', '/');

// Clear refresh token (must match original path)
clearCookie('refresh_token', '/api/refresh.php');


// =========================
// 🧹 DESTROY SESSION (if used)
// =========================
if (session_status() === PHP_SESSION_ACTIVE) {
    session_unset();
    session_destroy();
}


// =========================
// ✅ RESPONSE
// =========================
respond([
    'success' => true,
    'message' => 'Logged out successfully'
]);

$conn->close();