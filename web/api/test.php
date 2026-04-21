<?php
include_once "config.php";
include_once "utility.php";
echo json_encode(sendMail($EMAIL_API_KEY, "miliyarijohn1@gmail.com", "Miliya", "Test Email", ".<h3>Dear <b>Theeta Miliya</b></h3><br><p>teenu thoori erinodii</p>"));