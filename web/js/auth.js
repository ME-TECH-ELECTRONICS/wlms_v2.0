$(document).ready(function () {

    async function apiRequest({ url, method = "GET", data = null, timeout = 5000, retry = true }) {
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
            if (xhr.status === 401 && retry) {
                const refreshed = await refreshAccessToken();
                if (refreshed) {
                    return await apiRequest({
                        url,
                        method,
                        data,
                        timeout,
                        retry: false
                    });
                }
            }
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
        value = value.replace(/^\s/, '');
        $(this).val(value);
    });


    $('#loginForm').on('submit', function (e) {
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

        

        $.ajax({
            url: "/api/login.php",
            method: "POST",
            contentType: "application/json",
            dataType: "json",
            data: JSON.stringify({ email, password, captcha }),

            beforeSend: () => {
                $("#loginBtn")
                    .prop("disabled", true)
                    .addClass('fade')
                    .html('<i class="fa-solid fa-circle-notch fa-spin"></i> Logging in...');
            },

            success: function (res) {
                if (res.success) {
                    showMsg('Login successful. Redirecting...', 'success');
                    if (res.devices) localStorage.setItem('deviceId', res.devices);

                    $('#loginForm')[0].reset();
                    setTimeout(() => location.href = "/dashboard", 2000);
                } else {
                    showMsg(res.message, 'error');
                }
                resetBtn('loginBtn', 'Login');
                turnstile.reset();
            },

            error: (xhr) => {
                resetBtn('loginBtn', 'Login');
                turnstile.reset();

                let msg = xhr.responseJSON?.message || 'Server error';
                showMsg(msg, 'error');
            }
        });
    });

    $('#registerForm').on('submit', function (e) {
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
        if (pass1.length < 6) {
            return showMsg('Password must be at least 6 characters', 'error');
        }
        if (!captcha) {
            return showMsg('Complete captcha', 'error');
        }
        $.ajax({
            url: "/api/register.php",
            method: "POST",
            contentType: "application/json",
            dataType: "json",
            data: JSON.stringify({ name, email, password: pass1, captcha }),

            beforeSend: () => {
                $("#regBtn")
                    .prop("disabled", true)
                    .addClass('fade')
                    .html('<i class="fa-solid fa-circle-notch fa-spin"></i> Registering...');
            },

            success: function (res) {
                if (res.success) {
                    showMsg('Registered successfully. Proceed to login', 'success');

                    $('#registerForm')[0].reset();
                    setTimeout(() => {
                        $('.tab[data="loginForm"]').click();
                    }, 1500);
                } else {
                    showMsg(res.message, 'error');
                }
                resetBtn('regBtn', 'Register');
                turnstile.reset();
            },

            error: (xhr) => {
                resetBtn('regBtn', 'Register');
                turnstile.reset();

                let msg = xhr.responseJSON?.message || 'Server error';
                showMsg(msg, 'error');
            }
        });
    });

    $('#forgotForm').on('submit', function (e) {
        e.preventDefault();

        let email = $('#forgotEmail').val().trim();
        let captcha = $('#forgotForm input[name="cf-turnstile-response"]').val();
        if (!email) {
            return showMsg('Enter email', 'error');
        }

        // if (!captcha) {
        //     return showMsg('Complete captcha', 'error');
        // }

        $.ajax({
            url: "/api/request_password_reset.php",
            method: "POST",
            contentType: "application/json",
            dataType: "json",
            data: JSON.stringify({ email, captcha }),

            beforeSend: () => $("#forgotBtn").prop("disabled", true).addClass('fade').html('<i class="fa-solid fa-circle-notch fa-spin fa-xl"></i>'),

            success: function (res) {
                if (res.success) {
                    showMsg('Reset link sent successfully', 'success');
                } else {
                    $("#forgotBtn").prop("disabled", false).removeClass('fade').text('Send Reset Link');
                    showMsg(res.message, 'error');
                }
                $('#forgotForm')[0].reset();
                turnstile.reset();
            },

            error: () => {
                $("#forgotBtn").prop("disabled", false).removeClass('fade').text('Send Reset Link');
                showMsg('Server error', 'error');
            }
        });
    });

    function showPanel(id) {
        $(".panel").removeClass("active");
        $("#" + id).addClass("active");

        $(".tab").removeClass("active");
        $('.tab[data="' + id + '"]').addClass("active");
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

function resetBtn(id, text) {
    $("#" + id)
        .prop("disabled", false)
        .removeClass('fade')
        .text(text);
}