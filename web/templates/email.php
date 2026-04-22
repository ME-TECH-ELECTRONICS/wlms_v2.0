<?php

$FORGET_PASSWORD_EMAIL_TEMPLATE = 
'<div style="font-family:Segoe UI,Arial,sans-serif;background:#f4f6f8;padding:30px;">
    <div style="max-width:520px;margin:auto;background:#ffffff;border-radius:10px;padding:30px;">
        <h2 style="color:#ef4444;margin-bottom:10px;">Password Reset Request 🔐</h2>
        <p style="color:#333;font-size:14px;"> Hi <strong>{{name}}</strong>,</p>
        <p style="color:#555;font-size:14px;">We received a request to reset your password. Click the button below to set a new password.</p>
        <div style="margin:25px 0;text-align:center;">
            <a href="https://{{domain}}/reset-password?token={{token}}" target="_blank" style="background:#ef4444;color:#fff;padding:12px 20px;border-radius:6px;text-decoration:none;font-weight:bold;">Reset Password</a>
        </div>
        <p style="color:#777;font-size:13px;">This link will expire in <strong>15 minutes</strong> for security reasons.</p>
        <p style="color:#777;font-size:12px;">If you did not request a password reset, you can safely ignore this email.</p>
        <hr style="margin:20px 0;border:none;border-top:1px solid #eee;">
        <p style="font-size:11px;color:#999;">WLMS Support<br>This is an automated message, please do not reply.</p>
    </div>
</div>'; //{{name}}, {{domain}}, {{token}}

$VERIFICATION_EMAIL_TEMPLATE = 
'<div style="font-family:Segoe UI,Arial,sans-serif;background:#f4f6f8;padding:30px;">
    <div style="max-width:520px;margin:auto;background:#ffffff;border-radius:10px;padding:30px;">
    <h2 style="color:#4f46e5;margin-bottom:10px;">Welcome to WLMS 🎉</h2>
    <p style="color:#333;font-size:14px;">Hi <strong> {{name}} </strong>,</p>
    <p style="color:#555;font-size:14px;">Your account has been successfully created. Please verify your account to start using the platform. This link will expire in <strong>1 hour</strong>.</p>
    <div style="margin:25px 0;text-align:center;"><a href="https://{{domain}}/verify?token={{token}}" target="_blank" style="background:#4f46e5;color:#fff;padding:12px 20px;border-radius:6px;text-decoration:none;font-weight:bold;">Verify Your Account</a></div>
        <p style="color:#777;font-size:12px;">If you did not create this account, please ignore this email.</p>
        <hr style="margin:20px 0;border:none;border-top:1px solid #eee;">
        <p style="font-size:11px;color:#999;">WLMS Support<br> This is an automated message, please do not reply.</p>
    </div>
</div>'; //{{name}}, {{domain}}, {{token}}