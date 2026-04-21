<?php
require __DIR__ . '/vendor/autoload.php';

use Dotenv\Dotenv;

$dotenv = Dotenv::createImmutable(__DIR__ . '/../');
$dotenv->load();

//validate cloudeflare turnstile captcha
function validateCaptcha($token)
{
    $secretKey = "0x4AAAAAAC_Slvp99Zosbu2bIdpCOeuBQ1c";
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

function sendMail($email, $name, $subject, $message)
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

    curl_setopt($ch, CURLOPT_HTTPHEADER, [
        "api-key: YOUR_BREVO_API_KEY",
        "Content-Type: application/json",
        'Accept: application/json'
    ]);

    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_POST, true);
    curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($data));

    $response = curl_exec($ch);
    curl_close($ch);
}
