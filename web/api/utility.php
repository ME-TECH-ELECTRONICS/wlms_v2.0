<?php
include_once "config.php";
//validate cloudeflare turnstile captcha
function validateCaptcha($token, $secretKey)
{
    $url = 'https://challenges.cloudflare.com/turnstile/v0/siteverify';

    $data = [
        'secret' => $secretKey,
        'response' => $token
    ];

    $options = [
        'http' => [
            'header' => "Content-type: application/x-www-form-urlencoded\r\n",
            'method' => 'POST',
            'content' => http_build_query($data)
        ]
    ];

    $context = stream_context_create($options);
    $response = file_get_contents($url, false, $context);

    if ($response === FALSE) {
        return ['success' => false, 'error-codes' => ['internal-error']];
    }
    $result = json_decode($response, true);
    return $result['success'] ?? false;
}

function sendMail($api_key, $email, $name, $subject, $message)
{
    $url = "https://api.brevo.com/v3/smtp/email";

    $data = [
        "sender" => [
            "name" => "WLMS Support",
            "email" => "no-reply@metech-el.cc"
        ],
        "to" => [
            [
                "email" => $email,
                "name" => $name
            ]
        ],
        "subject" => $subject,
        "htmlContent" => $message
    ];

    $ch = curl_init($url);

    curl_setopt_array($ch, [
        CURLOPT_HTTPHEADER => [
            "api-key: " . $api_key,
            "Content-Type: application/json",
            "Accept: application/json"
        ],
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_POST => true,
        CURLOPT_POSTFIELDS => json_encode($data)
    ]);

    $response = curl_exec($ch);
    $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    $error = curl_error($ch);

    curl_close($ch);

    // Handle cURL error
    if ($error) {
        return [
            "success" => false,
            "message" => "cURL Error: " . $error
        ];
    }

    // Decode API response
    $responseData = json_decode($response, true);

    // Success check (Brevo returns 201 on success)
    if ($httpCode === 201) {
        return [
            "success" => true,
            "message" => "Email sent successfully",
            "data" => $responseData
        ];
    }

    return [
        "success" => false,
        "message" => $responseData['message'] ?? "Failed to send email",
        "status_code" => $httpCode,
        "data" => $responseData
    ];
}

function generateRandomToken($length = 32) {
    return bin2hex(random_bytes($length));
}

function simpleResponse($data) {
    header('Content-Type: application/json');
    echo json_encode($data);
    exit;
}