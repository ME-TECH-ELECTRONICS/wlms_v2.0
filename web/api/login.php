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
        simpleResponse(['success' => false, 'message' => 'Invalid JSON']);
    }

    $email = strtolower(trim($input['email'] ?? ''));
    $password = trim($input['password'] ?? '');
    $captcha = trim($input['captcha'] ?? '');

    if (!$email || !$password || !$captcha) {
        simpleResponse(['success' => false, 'message' => 'Missing fields']);
    }

    // if (!validateCaptcha($captcha, $CAPTCHA_SECRET)) {
    //     simpleResponse(['success' => false, 'message' => 'Captcha failed']);
    // }

    if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
        simpleResponse(['success' => false, 'message' => 'Invalid email']);
    }


    $fetchUser = $conn->prepare("SELECT id, name, email, password, is_verified, verification_token, verification_expires, failed_attempts, lock_until, last_verification_sent FROM users WHERE email = ?");
    $fetchUser->bind_param("s", $email);
    if (!$fetchUser->execute()) {
        simpleResponse(['success' => false, 'message' => 'Database error']);
    }

    $result = $fetchUser->get_result();

    if ($result->num_rows > 0) {

        $user = $result->fetch_assoc();
        if ($user['lock_until'] && strtotime($user['lock_until']) > time()) {
            $remaining = strtotime($user['lock_until']) - time();

            simpleResponse([
                'success' => false,
                'message' => 'Account locked. Try again in ' . ceil($remaining / 60) . ' minutes.'
            ]);
        }
        if (password_verify($password, $user['password'])) {
            $updateUser = $conn->prepare("UPDATE users SET failed_attempts = 0, lock_until = NULL WHERE id = ?");
            $updateUser->bind_param("i", $user['id']);

            if (!$updateUser->execute()) {
                simpleResponse(['success' => false, 'message' => 'Database error']);
            }
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
                        simpleResponse([
                            'success' => false,
                            'message' => 'Please wait before requesting another verification email.'
                        ]);
                    }
                    // Update DB
                    $updateVerification = $conn->prepare("
                        UPDATE users 
                        SET verification_token = ?, verification_expires = ?, last_verification_sent = NOW()
                        WHERE id = ? AND (last_verification_sent IS NULL OR last_verification_sent < NOW() - INTERVAL 60 SECOND)
                    ");
                    $updateVerification->bind_param("ssi", $hashedToken, $expires, $user['id']);

                    if (!$updateVerification->execute()) {
                        simpleResponse(['success' => false, 'message' => 'Database error']);
                    }
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

                    simpleResponse([
                        'success' => false,
                        'message' => 'Please verify your email. A new verification link has been sent if the previous one expired.'
                    ]);
                } else {
                    simpleResponse([
                        'success' => false,
                        'message' => 'Please verify your email before logging in.'
                    ]);
                }
            }
            $jti = bin2hex(random_bytes(16));
            // ✅ Access Token (short life)
            $accessPayload = [
                "iss" => $JWT_ISSUER,
                "iat" => time(),
                "nbf" => time(),
                "exp" => time() + $ACCESS_TOKEN_EXP, // 1 hour
                "data" => [
                    "id" => $user['id'],
                    "email" => $user['email']
                ],
                "jti" => $jti
            ];

            $accessToken = JWT::encode($accessPayload, $JWT_SECRET, 'HS256');

            // 🔄 Refresh Token (long life)
            $refreshToken = bin2hex(random_bytes(32));
            $refreshTokenHash = hash('sha256', $refreshToken);
            $refreshExpires = date("Y-m-d H:i:s", time() + $REFRESH_TOKEN_EXP); // 7 days

            // ✅ Store HASH in DB (NOT raw token)
            $insertToken = $conn->prepare("INSERT INTO user_tokens (user_id, refresh_token_hash, expires_at) VALUES (?, ?, ?)");
            $insertToken->bind_param("iss", $user['id'], $refreshTokenHash, $refreshExpires);
            if (!$insertToken->execute()) {
                simpleResponse(['success' => false, 'message' => 'Database error']);
            }

            // 🍪 Access token cookie
            setcookie("auth_token", $accessToken, [
                'expires' => time() + $ACCESS_TOKEN_EXP,
                'path' => '/',
                'secure' => true,
                'httponly' => true,
                'samesite' => 'Strict'
            ]);

            // 🍪 Refresh token cookie
            setcookie("refresh_token", $refreshToken, [
                'expires' => time() + $REFRESH_TOKEN_EXP,
                'path' => '/api/refresh.php',
                'secure' => true,
                'httponly' => true,
                'samesite' => 'Strict'
            ]);

            // STEP 1: get IDs of old tokens (keep latest 5)
            $cleanup = $conn->prepare("SELECT id FROM user_tokens WHERE user_id = ? ORDER BY created_at DESC LIMIT 100 OFFSET 5");

            if (!$cleanup) {
                die("Prepare failed: " . $conn->error);
            }

            $cleanup->bind_param("i", $user['id']);
            $cleanup->execute();
            if (!$cleanup->execute()) {
                simpleResponse(['success' => false, 'message' => 'Database error']);
            }
            $result = $cleanup->get_result();

            $ids = [];
            while ($row = $result->fetch_assoc()) {
                $ids[] = $row['id'];
            }

            $cleanup->close();


            // STEP 2: delete them if any exist
            if (!empty($ids)) {

                // Build placeholders (?, ?, ?, ...)
                $placeholders = implode(',', array_fill(0, count($ids), '?'));

                $types = str_repeat('i', count($ids));

                $delete = $conn->prepare("DELETE FROM user_tokens WHERE id IN ($placeholders)");

                if (!$delete) {
                    die("Prepare failed: " . $conn->error);
                }

                $delete->bind_param($types, ...$ids);
                $delete->execute();
                $delete->close();
            }

            $deviceInfo = $conn->prepare("SELECT * FROM user_devices WHERE user_id = ?");
            $deviceInfo->bind_param("i", $user['id']);
            if (!$deviceInfo->execute()) {
                simpleResponse(['success' => false, 'message' => 'Database error']);
            }
            $result = $deviceInfo->get_result();
            if ($result->num_rows > 0) {
                $row = $result->fetch_assoc();
                simpleResponse([
                    'success' => true,
                    'message' => 'Login successful',
                    'devices' => $row['device_id']
                ]);
            } else {
                simpleResponse([
                    'success' => true,
                    'message' => 'Login successful'
                ]);
            }
            $fetchUser->close();
            $updateUser->close();
        } else {
            $failedAttempts = ((int)$user['failed_attempts']) + 1;
            $lockUntil = null;

            // Lock if limit exceeded
            if ($failedAttempts >= $MAX_ATTEMPTS) {
                $lockUntil = date("Y-m-d H:i:s", time() + $LOCK_TIME);
            }

            // Update DB
            $updateUser = $conn->prepare("
                UPDATE users 
                SET failed_attempts = ?, last_failed_login = NOW(), lock_until = ?
                WHERE id = ?
            ");
            $updateUser->bind_param("isi", $failedAttempts, $lockUntil, $user['id']);

            if (!$updateUser->execute()) {
                simpleResponse(['success' => false, 'message' => 'Database error']);
            }
            $fetchUser->close();
            $updateUser->close();
            simpleResponse([
                'success' => false,
                'message' => $failedAttempts >= $MAX_ATTEMPTS
                    ? 'Account locked for ' . ($LOGIN_LOCK_TIME / 60) . ' minutes.'
                    : 'Invalid credentials'
            ]);
        }
    } else {
        $fetchUser->close();
        simpleResponse(['success' => false, 'message' => 'Invalid credentials']);
    }
} else {
    simpleResponse(['success' => false, 'message' => 'Invalid request method']);
}
