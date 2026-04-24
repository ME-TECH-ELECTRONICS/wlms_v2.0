<?php
include_once 'config.php';


$conn = new mysqli($DB_HOST, $DB_USER, $DB_PASS, $DB_NAME);
if (!$conn) {
    die("{'success': false, 'message': 'Database Error " . $conn->connect_error . "'}");
}