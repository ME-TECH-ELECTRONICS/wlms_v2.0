$(document).ready(function () {
    const icon = $("#verifyIcon");
    const title = $("#verifyTitle");
    const desc = $("#verifyDesc");
    const btn = $("#verifyBtn");

    function setLoading() {
        icon.html('<i class="fa-solid fa-circle-notch fa-spin"></i>').css("color", "#3b82f6");
        title.text("Verifying Account...").css("color", "#3b82f6");
        desc.text("Please wait while we verify your email.");
        btn.hide();
    }

    function setSuccess(msg) {
        icon.html('<i class="fa-solid fa-circle-check"></i>').css("color", "#22c55e");
        title.text("Account Verified").css("color", "#22c55e");
        desc.text(msg);
        btn.show();
    }

    function setError(msg) {
        icon.html('<i class="fa-solid fa-circle-xmark"></i>').css("color", "#ef4444");
        title.text("Verification Failed").css("color", "#ef4444");
        desc.text(msg);
        btn.show().text("Back to Login");
    }

    const token = new URLSearchParams(location.search).get("token");

    if (!token) {
        setError("Invalid or missing verification link.");
        return;
    }

    setLoading();

    $.ajax({
        url: `/api/verify.php?token=${encodeURIComponent(token)}`,
        method: "GET",
        dataType: "json",

        success: function (res) {
            if (res.success) {
                setSuccess(res.message);
            } else {
                setError(res.message);
            }
        },

        error: function () {
            setError("Server error. Please try again later.");
        }
    });

});