<?php

require_once __DIR__ . '/../api/config.php';
require_once __DIR__ . '/../api/utility.php';
require_once __DIR__ . '/../templates/email.php';
header("Content-Type: application/json");

if ($_SERVER["REQUEST_METHOD"] !== "POST") {
    simpleResponse([
        "success" => false,
        "message" => "Invalid request method"
    ], 405);
}

$data = json_decode(file_get_contents("php://input"), true);

$token = trim($data["token"] ?? "");
$password = trim($data["password"] ?? "");
$confirmPassword = trim($data["confirmPassword"] ?? "");

if (empty($token) || empty($password) || empty($confirmPassword)) {
    simpleResponse([
        "success" => false,
        "message" => "All fields are required"
    ], 400);
}

if ($password !== $confirmPassword) {
    simpleResponse([
        "success" => false,
        "message" => "Passwords do not match"
    ], 400);
}

if (strlen($password) < 8) {
    simpleResponse([
        "success" => false,
        "message" => "Password must be at least 8 characters"
    ], 400);
}

try {

    $tokenHash = hash("sha256", $token);
    $stmt = $conn->prepare("SELECT id, user_id, email, name FROM password_resets WHERE token_hash = ? AND used = 0 AND expires_at > NOW() LIMIT 1");
    $stmt->bind_param("s", $tokenHash);
    $stmt->execute();
    $result = $stmt->get_result();
    $reset = $result->fetch_assoc();
    if (!$reset) {
        simpleResponse([
            "success" => false,
            "message" => "Invalid or expired token"
        ], 400);
    }
    $passwordHash = password_hash($password, PASSWORD_DEFAULT);
    $updateStmt = $conn->prepare("
        UPDATE users
        SET password = ?
        WHERE id = ?
    ");

    $updateStmt->bind_param(
        "si",
        $passwordHash,
        $reset["user_id"]
    );

    $updateStmt->execute();

    // Mark token as used
    $usedStmt = $conn->prepare("
        UPDATE password_resets
        SET used = 1
        WHERE id = ?
    ");

    $usedStmt->bind_param("i", $reset["id"]);
    $usedStmt->execute();
    $emailContent = str_replace(
        ["{{name}}", "{{domain}}"],
        [$reset["name"], $DOMAIN_NAME],
        $PASSWORD_CHANGED_EMAIL_TEMPLATE
    );
    sendMail($EMAIL_API_KEY, $reset["email"], "Password Changed", $reset["name"], $emailContent);
    simpleResponse([
        "success" => true,
        "message" => "Password changed successfully"
    ]);

} catch (Exception $e) {
    simpleResponse([
        "success" => false,
        "message" => "Server error"
    ], 500);
}