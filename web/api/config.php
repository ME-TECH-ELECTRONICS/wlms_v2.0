<?php
require __DIR__ . '/../vendor/autoload.php';
use Dotenv\Dotenv;
$dotenv = Dotenv::createImmutable(__DIR__ . '/../');
$dotenv->load();

$DB_HOST = $_ENV['DB_HOST'] ?? null;
$DB_NAME = $_ENV['DB_NAME'] ?? null;
$DB_USER = $_ENV['DB_USER'] ?? null;
$DB_PASS = $_ENV['DB_PASS'] ?? null;
$JWT_SECRET = $_ENV['JWT_SECRET'] ?? null;
$JWT_ISSUER = $_ENV['JWT_ISSUER'] ?? null;
$CAPTCHA_SECRET = $_ENV['CAPTCHA_SECRET'] ?? null;
$EMAIL_API_KEY = $_ENV['EMAIL_API_KEY'] ?? null;
$DOMAIN_NAME = $_ENV['DOMAIN_NAME'] ?? null;

$MAX_LOGIN_ATTEMPTS = 5;
$LOGIN_LOCK_TIME = 300;