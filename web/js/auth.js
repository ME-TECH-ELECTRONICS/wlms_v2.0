$(document).ready(function () {
    const urlParams = new URLSearchParams(window.location.search);
    const token = urlParams.get('token');
    if (token) {
        $('#resetToken').val(token);
    }
    $(".tab, .switch").click(function () {
        showPanel($(this).attr("data"));
    });

    $(".pass-toggle").click(function () {
        let id = $(this).attr("data");
        let input = $("#" + id);

        if (input.attr("type") === "password") {
            input.attr("type", "text");
            $(this).text("Hide");
        } else {
            input.attr("type", "password");
            $(this).text("Show");
        }
    });

    function showMsg(message, type = 'success') {
        let toast = $(`<div class="toast ${type}">${message}</div>`);
        $("#toastBox").append(toast);
        setTimeout(() => {
            toast.fadeOut(300, function () {
                $(this).remove();
            });
        }, 3000);
    }

    function btnState(id, text, loading = false) {
        const $btn = $("#" + id);
        $btn.prop("disabled", loading);
        if (loading) {
            $btn.addClass("fade").html(`<i class="fa-solid fa-circle-notch fa-spin"></i> ${text}`);
        } else {
            $btn.removeClass("fade").text(text);
        }
    }
    function showPanel(id) {
        $(".panel").removeClass("active");
        $("#" + id).addClass("active");

        $(".tab").removeClass("active");
        $('.tab[data="' + id + '"]').addClass("active");
    }

    async function apiRequest({ url, method = "GET", data = null, timeout = 5000 }) {
        try {
            const options = {
                url,
                method,
                timeout,
                dataType: "json"
            };

            if (data !== null) {
                options.contentType = "application/json";
                options.data = JSON.stringify(data);
            }
            return await $.ajax(options);
        } catch (xhr) {
            if (xhr.status === 429) {
                showMsg(xhr.responseJSON?.message || 'Too many requests. Please try again later.', 'error');
                return;
            }
            console.error("API request error:", xhr);
            throw xhr;
        }
    }

    $('input[type=email]').on('input', function (e) {
        let value = $(this).val();
        value = value.replace(/[^a-zA-Z0-9@._+-]/g, '');
        if (value.length > 64) {
            value = value.substring(0, 64);
        }
        let parts = value.split('@');
        if (parts.length > 2) {
            value = parts[0] + '@' + parts.slice(1).join('').replace(/@/g, '');
        }
        $(this).val(value);
    });

    $('input[type=password]').on('input', function () {
        let value = $(this).val();
        value = value.replace(/\s/g, '');
        if (value.length > 20) {
            value = value.substring(0, 20);
        }
        $(this).val(value);
    });

    $('input[name="fullname"]').on('input', function () {
        let value = $(this).val();
        value = value.replace(/[^a-zA-Z ]/g, '');
        value = value.replace(/\s+/g, ' ');
        value = value.replace(/^\s+/, '');
        $(this).val(value);
    });


    $('#loginForm').on('submit', async function (e) {
        e.preventDefault();

        let email = $('#loginEmail').val().trim();
        let password = $('#loginPass').val();
        let captcha = $('#loginForm input[name="cf-turnstile-response"]').val();

        if (!email || !password) {
            return showMsg('All fields required', 'error');
        }

        if (!captcha) {
            return showMsg('Complete captcha', 'error');
        }
        btnState('loginBtn', 'Logging in...', true);
        try {
            const res = await apiRequest({
                url: "/api/login.php",
                method: "POST",
                data: { email, password, captcha }
            });
            if (res.success) {
                showMsg('Login successful. Redirecting...', 'success');
                if (res.devices) localStorage.setItem('deviceId', res.devices);
                $('#loginForm')[0].reset();
                setTimeout(() => location.href = "/dashboard", 2000);
            } else {
                showMsg(res.message, 'error');
            }
            btnState('loginBtn', 'Login');
            turnstile.reset();
        } catch (xhr) {
            btnState('loginBtn', 'Login');
            turnstile.reset();
            let msg = xhr.responseJSON?.message || 'Server error';
            showMsg(msg, 'error');
            return;
        }
    });

    $('#registerForm').on('submit', async function (e) {
        e.preventDefault();

        let name = $('#regName').val().trim();
        let email = $('#regEmail').val().trim();
        let pass1 = $('#regPass1').val();
        let pass2 = $('#regPass2').val();
        let captcha = $('#registerForm input[name="cf-turnstile-response"]').val();

        if (!name || !email || !pass1 || !pass2) {
            return showMsg('All fields required', 'error');
        }
        if (pass1 !== pass2) {
            return showMsg('Passwords do not match', 'error');
        }
        if (pass1.length < 8) {
            return showMsg('Password must be at least 8 characters', 'error');
        }
        if (!captcha) {
            return showMsg('Complete captcha', 'error');
        }
        btnState('regBtn', 'Registering...', true);
        try {
            const res = await apiRequest({
                url: "/api/register.php",
                method: "POST",
                data: { name, email, password: pass1, captcha }
            });
            if (res.success) {
                showMsg('Registered successfully. Proceed to login', 'success');
                $('#registerForm')[0].reset();
                setTimeout(() => { $('.tab[data="loginForm"]').click(); }, 1500);
            } else {
                showMsg(res.message, 'error');
            }
            btnState('regBtn', 'Register');
            turnstile.reset();
        } catch (xhr) {
            let msg = xhr.responseJSON?.message || 'Server error';
            showMsg(msg, 'error');
            btnState('regBtn', 'Register');
            turnstile.reset();
            return;
        }
    });

    $('#forgotForm').on('submit', async function (e) {
        e.preventDefault();

        let email = $('#forgotEmail').val().trim();
        let captcha = $('#forgotForm input[name="cf-turnstile-response"]').val();
        if (!email) {
            return showMsg('Enter email', 'error');
        }

        if (!captcha) {
            return showMsg('Complete captcha', 'error');
        }
        btnState('forgotBtn', 'Sending...', true);
        try {
            const res = await apiRequest({
                url: "/api/request_password_reset.php",
                method: "POST",
                data: { email, captcha }
            });
            if (res.success) {
                showMsg('Reset link sent successfully', 'success');
                $('#forgotForm')[0].reset();
            } else {
                showMsg(res.message, 'error');
            }
            btnState('forgotBtn', 'Send Reset Link');
            turnstile.reset();
        } catch (xhr) {
            let msg = xhr.responseJSON?.message || 'Server error';
            showMsg(msg, 'error');
            btnState('forgotBtn', 'Send Reset Link');
            turnstile.reset();
            return;
        }
    });

    $(document).on('submit', '#passwordResetForm', async function (e) {
        e.preventDefault();
        let token = $('#resetToken').val().trim();
        let newPass = $('#newPass').val();
        let confirmPass = $('#confirmPass').val();
        let captcha = $('#passwordResetForm input[name="cf-turnstile-response"]').val();
        if (!token || !newPass || !confirmPass) {
            return showMsg('All fields required', 'error');
        }
        if (newPass !== confirmPass) {
            return showMsg('Passwords do not match', 'error');
        }
        if (newPass.length < 8) {
            return showMsg('Password must be at least 8 characters', 'error');
        }
        if (!captcha) {
            return showMsg('Complete captcha', 'error');
        }
        btnState('resetBtn', 'Resetting...', true);
        try {
            const res = await apiRequest({
                url: "/api/reset_password.php",
                method: "POST",
                data: { token, password: newPass, confirmPassword: confirmPass, captcha }
            });
            if (res.success) {
                showMsg('Password reset successful. Redirecting to login...', 'success');
                $('#passwordResetForm')[0].reset();
                setTimeout(() => location.href = "/auth", 2000);
            } else {
                showMsg(res.message, 'error');
            }
            btnState('resetBtn', 'Reset Password');
            turnstile.reset();
        } catch (xhr) {
            let msg = xhr.responseJSON?.message || 'Server error';
            showMsg(msg, 'error');
            btnState('resetBtn', 'Reset Password');
            turnstile.reset();
            return;
        }
    });
});
