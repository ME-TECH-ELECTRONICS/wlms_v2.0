<?php
    $host = "localhost";
    $db = "wlms";
    $user = "root";
    $password = "";

    //connect mysql server
    $conn = new mysqli($host, $user, $password, $db);
    if(!$conn) {
        die("{'success': false, 'message': 'Database Error " . $conn->connect_error . "'}");
    }
?>