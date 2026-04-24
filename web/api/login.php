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

    $email = strtolower(trim($input['email'] ?? ''));
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
   

    $stmt = $conn->prepare("SELECT id, name, email, password, is_verified, verification_token, verification_expires, failed_attempts, lock_until, last_verification_sent FROM users WHERE email = ?");
    $stmt->bind_param("s", $email);
    $stmt->execute();

    $result = $stmt->get_result();

    if ($result->num_rows > 0) {

        $user = $result->fetch_assoc();
        if ($user['lock_until'] && strtotime($user['lock_until']) > time()) {
            $remaining = strtotime($user['lock_until']) - time();

            echo json_encode([
                'success' => false,
                'message' => 'Account locked. Try again in ' . ceil($remaining / 60) . ' minutes.'
            ]);
            exit;
        }
        if (password_verify($password, $user['password'])) {
            $stmt2 = $conn->prepare("UPDATE users SET failed_attempts = 0, lock_until = NULL WHERE id = ?");
            $stmt2->bind_param("i", $user['id']);
            $stmt2->execute();
            if (!$user['is_verified']) {
                // Check expiry
                $expired = !$user['verification_expires'] || strtotime($user['verification_expires']) < time();

                if ($expired) {
                    // 🔄 Generate new token
                    $token = generateRandomToken();
                    $hashedToken = hash('sha256', $token);
                    $expires = date("Y-m-d H:i:s", time() + 3600);
                    $lastSent = $user['last_verification_sent'] ?? null;

                    if ($lastSent && (time() - strtotime($lastSent)) < 60) {
                        echo json_encode([
                            'success' => false,
                            'message' => 'Please wait before requesting another verification email.'
                        ]);
                        exit;
                    }
                    // Update DB
                    $stmt2 = $conn->prepare("
                        UPDATE users 
                        SET verification_token = ?, verification_expires = ?, last_verification_sent = NOW()
                        WHERE id = ? AND (last_verification_sent IS NULL OR last_verification_sent < NOW() - INTERVAL 60 SECOND)
                    ");
                    $stmt2->bind_param("ssi", $hashedToken, $expires, $user['id']);
                    $stmt2->execute();

                    // 📧 Send email
                    $message = str_replace(
                        ['{{name}}', '{{domain}}', '{{token}}'],
                        [
                            htmlspecialchars($user['name'], ENT_QUOTES, 'UTF-8'),
                            $DOMAIN_NAME,
                            $token
                        ],
                        $VERIFICATION_EMAIL_TEMPLATE
                    );

                    sendMail($EMAIL_API_KEY, $user['email'], $user['name'], "Verify Your Account", $message);

                    echo json_encode([
                        'success' => false,
                        'message' => 'Please verify your email. A new verification link has been sent if the previous one expired.'
                    ]);
                } else {
                    echo json_encode([
                        'success' => false,
                        'message' => 'Please verify your email before logging in.'
                    ]);
                }
                exit;
            }

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
            $stmt->close();
            exit;
        } else {
            $failedAttempts = ((int)$user['failed_attempts']) + 1;
            $lockUntil = null;

            // Lock if limit exceeded
            if ($failedAttempts >= $MAX_ATTEMPTS) {
                $lockUntil = date("Y-m-d H:i:s", time() + $LOCK_TIME);
            }

            // Update DB
            $stmt2 = $conn->prepare("
                UPDATE users 
                SET failed_attempts = ?, last_failed_login = NOW(), lock_until = ?
                WHERE id = ?
            ");
            $stmt2->bind_param("isi", $failedAttempts, $lockUntil, $user['id']);
            $stmt2->execute();

            echo json_encode([
                'success' => false,
                'message' => $failedAttempts >= $MAX_ATTEMPTS
                    ? 'Account locked for ' . ($LOGIN_LOCK_TIME / 60) . ' minutes.'
                    : 'Invalid credentials'
            ]);
            $stmt->close();
            $stmt2->close();
            exit;
        }

    } else {
        echo json_encode(['success' => false, 'message' => 'Invalid credentials']);
        $stmt->close();
        exit;
    }
    
    

} else {
    echo json_encode(['success' => false, 'message' => 'Invalid request method']);
    exit;
}