<?php
require __DIR__ . '/../vendor/autoload.php';

include_once 'config.php';
include_once 'db.php';
include_once 'utility.php';

use Firebase\JWT\JWT;

header('Content-Type: application/json');


if ($_SERVER['REQUEST_METHOD'] === 'POST') {

    $input = json_decode(file_get_contents("php://input"), true);

    if (!$input) {
        echo json_encode(['success' => false, 'message' => 'Invalid JSON']);
        exit;
    }

    $email = filter_var(trim($input['email'] ?? ''), FILTER_SANITIZE_EMAIL);
    $password = trim($input['password'] ?? '');
    $captcha = trim($input['captcha'] ?? '');

    if (!$email || !$password || !$captcha) {
        echo json_encode(['success' => false, 'message' => 'Missing fields']);
        exit;
    }

     if (!validateCaptcha($captcha, $CAPTCHA_SECRET)) {
        echo json_encode(['success' => false, 'message' => 'Captcha failed']);
        exit;
    }

    if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
        echo json_encode(['success' => false, 'message' => 'Invalid email']);
        exit;
    }
   

    $stmt = $conn->prepare("SELECT id, name, email, password FROM users WHERE email = ?");
    $stmt->bind_param("s", $email);
    $stmt->execute();

    $result = $stmt->get_result();

    if ($result->num_rows > 0) {

        $user = $result->fetch_assoc();

        if (password_verify($password, $user['password'])) {

            // ✅ Create JWT payload
            $payload = [
                "iss" => $JWT_ISSUER,
                "iat" => time(),
                "nbf" => time(),
                "exp" => time() + 86400, // 1 day
                "data" => [
                    "id" => $user['id'],
                    "name" => $user['name'],
                    "email" => $user['email']
                ]
            ];

            // 🔑 Generate token
            $jwt = JWT::encode($payload, $JWT_SECRET, 'HS256');

            // 🍪 Store in secure cookie
            setcookie("auth_token", $jwt, [
                'expires' => time() + 86400,
                'path' => '/',
                'secure' => true,     // HTTPS only
                'httponly' => true,   // JS cannot access
                'samesite' => 'Strict'
            ]);

            echo json_encode([
                'success' => true,
                'message' => 'Login successful'
            ]);

        } else {
            echo json_encode(['success' => false, 'message' => 'Invalid credentials']);
        }

    } else {
        echo json_encode(['success' => false, 'message' => 'Invalid credentials']);
    }

    $stmt->close();

} else {
    echo json_encode(['success' => false, 'message' => 'Invalid request method']);
}