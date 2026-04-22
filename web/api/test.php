<?php
include_once "config.php";
include_once "utility.php";
$message = 'test email from wlms';

echo json_encode(sendMail($EMAIL_API_KEY, "melvin.rt.123@gmail.com", "Miliya", "Test Email", $message));