<?php

require_once __DIR__ . '/../api/config.php';
require_once __DIR__ . '/../api/utility.php';
include_once __DIR__ . '/../templates/email.php';

header("Content-Type: application/json");

if ($_SERVER["REQUEST_METHOD"] !== "POST") {
    simpleResponse([
        "success" => false,
        "message" => "Invalid request method"
    ], 405);
}

$data = json_decode(file_get_contents("php://input"), true);
$email = trim($data["email"] ?? "");
if (empty($email)) {
    simpleResponse([
        "success" => false,
        "message" => "Email is required"
    ], 400);
}
if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
    simpleResponse([
        "success" => false,
        "message" => "Invalid email address"
    ], 400);
}

try {
    $stmt = $conn->prepare("
        SELECT id, email, name
        FROM users
        WHERE email = ?
        LIMIT 1
    ");
    $stmt->bind_param("s", $email);
    $stmt->execute();
    $result = $stmt->get_result();
    $user = $result->fetch_assoc();
    $username = $user["name"] ?? "User";
    if (!$user) {
        simpleResponse([
            "success" => true,
            "message" => "If the email exists, a reset link has been sent."
        ]);
    }
    $cooldownStmt = $conn->prepare("SELECT created_at FROM password_resets WHERE user_id = ? AND used = 0 ORDER BY created_at DESC LIMIT 1");

    $cooldownStmt->bind_param("i", $user["id"]);
    $cooldownStmt->execute();

    $cooldownResult = $cooldownStmt->get_result();
    $lastReset = $cooldownResult->fetch_assoc();

    if ($lastReset) {
        $lastRequestTime = strtotime($lastReset["created_at"]);
        $currentTime = time();
        $remaining = $PASSWORD_RESET_COOLDOWN - ($currentTime - $lastRequestTime);
        if ($remaining > 0) {
            $minutes = ceil($remaining / 60);
            simpleResponse([
                "success" => false,
                "message" => "Please wait {$minutes} minute(s) before requesting another reset email."
            ], 429);
        }
    }
    $token = generateRandomToken();
    $tokenHash = hash("sha256", $token);
    $expiresAt = date("Y-m-d H:i:s", time() + $PASSWORD_RESET_EXPIRY);
    $deleteStmt = $conn->prepare("
        DELETE FROM password_resets
        WHERE user_id = ?
    ");
    $deleteStmt->bind_param("i", $user["id"]);
    $deleteStmt->execute();

    $insertStmt = $conn->prepare("INSERT INTO password_resets ( user_id, name, email, token_hash, expires_at ) VALUES (?, ?, ?, ?, ?)");
    $insertStmt->bind_param("issss", $user["id"], $user["name"], $user["email"], $tokenHash, $expiresAt);
    $insertStmt->execute();

    // Reset URL
    $emailContent = str_replace(
        ["{{name}}", "{{domain}}", "{{token}}"],
        [$username, $DOMAIN_NAME, $token],
        $FORGET_PASSWORD_EMAIL_TEMPLATE
    );
    sendMail($EMAIL_API_KEY, $user["email"], "Password Reset", $username, $emailContent);

    simpleResponse([
        "success" => true,
        "message" => "If the email exists, a reset link has been sent."
    ]);
} catch (Exception $e) {
    simpleResponse([
        "success" => false,
        "message" => "Server error"
    ], 500);
}
