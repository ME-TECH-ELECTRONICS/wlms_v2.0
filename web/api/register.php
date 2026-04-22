<?php
include_once 'config.php';
include_once 'db.php';
include_once 'utility.php';
include_once __DIR__ . '/../templates/email.php';


header('Content-Type: application/json');

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    echo json_encode(['success' => false, 'message' => 'Invalid request method']);
    exit;
}

// Get JSON input
$input = json_decode(file_get_contents("php://input"), true);

if (!$input) {
    echo json_encode(['success' => false, 'message' => 'Invalid JSON']);
    exit;
}

// Sanitize inputs
$name = trim($input['name'] ?? '');
$email = filter_var(trim($input['email'] ?? ''), FILTER_SANITIZE_EMAIL);
$password = trim($input['password'] ?? '');
$captcha = trim($input['captcha'] ?? '');

// Basic validation
if (!$name || !$email || !$password || !$captcha) {
    echo json_encode(['success' => false, 'message' => 'All fields required']);
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

// Optional: stronger validation
if (strlen($password) < 6) {
    echo json_encode(['success' => false, 'message' => 'Password must be at least 6 characters']);
    exit;
}

// Hash password
$hashedPassword = password_hash($password, PASSWORD_DEFAULT);

// Check if email already exists
$stmt = $conn->prepare("SELECT id FROM users WHERE email = ?");
$stmt->bind_param("s", $email);
$stmt->execute();
$result = $stmt->get_result();

if ($result->num_rows > 0) {
    echo json_encode(['success' => false, 'message' => 'Email already registered']);
    $stmt->close();
    exit;
}
$stmt->close();

$token = generateRandomToken(); // Generate a random token for email verification
$hashedToken = hash('sha256', $token);
$expires = date("Y-m-d H:i:s", time() + 3600); // Token expires in 1 hour

// Insert user
$stmt = $conn->prepare("INSERT INTO users (name, email, password, verification_token, verification_expires) VALUES (?, ?, ?, ?, ?)");
$stmt->bind_param("sssss", $name, $email, $hashedPassword, $hashedToken, $expires);

if ($stmt->execute()) {
    $message = str_replace(
        ['{{name}}', '{{domain}}', '{{token}}'],
        [htmlspecialchars($name, ENT_QUOTES, 'UTF-8'), $DOMAIN_NAME, $token],
        $VERIFICATION_EMAIL_TEMPLATE
    );
    sendMail($EMAIL_API_KEY, $email, $name, "Registration Successful", $message);
    echo json_encode([
        'success' => true,
        'message' => 'Registration successful'
    ]);
} else {
    echo json_encode([
        'success' => false,
        'message' => 'Registration failed'
    ]);
}

$stmt->close();
$conn->close();