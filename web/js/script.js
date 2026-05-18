$(document).ready(async function () {

    const deviceId = localStorage.getItem("deviceId");

    let last_seen = null;
    let refreshBusy = false;

    // Cached DOM elements
    const $motorState = $("#motorState");
    const $statusDot = $(".status-dot");
    const $sysStatus = $("#sysStatus");
    const $updateTimer = $("#updateTimer");
    const $deviceList = $("#deviceList");
    const $toastBox = $("#toastBox");
    const $clockPill = $("#clockPill");

    // =========================
    // INPUT CONFIG
    // =========================

    const inputs = [
        { id: "startThreshold", min: 1, max: 100, regex: /[^0-9]/g, type: "number" },
        { id: "stopThreshold", min: 1, max: 100, regex: /[^0-9]/g, type: "number" },
        { id: "dryRunTimeout", min: 1, max: 30, regex: /[^0-9]/g, type: "number" },
        { id: "dryRunCancel", min: 1, max: 20, regex: /[^0-9]/g, type: "number" },
        { id: "motorTimeout", min: 1, max: 20, regex: /[^0-9]/g, type: "number" },
        { id: "motorStartHour", min: 0, max: 23, regex: /[^0-9]/g, type: "number" },
        { id: "motorStopHour", min: 1, max: 23, regex: /[^0-9]/g, type: "number" },
        { id: "deviceId", min: 12, max: 12, regex: /[^a-fA-F0-9]/g, type: "text" },
        { id: "wifiSsid", min: 1, max: 32, regex: /[^ -~]/g, type: "text" },
        { id: "wifiPassword", min: 1, max: 32, regex: /[^ -~]/g, type: "text" }
    ];

    // =========================
    // INPUT VALIDATION
    // =========================

    for (const input of inputs) {
        const $field = $(`#${input.id}`);
        $field.on("input", function () {
            let value = $(this).val();
            value = value.replace(input.regex, "");
            if (input.type === "number") {
                if (value === "") {
                    $(this).val("");
                    return;
                }
                value = Number(value);
                if (!Number.isFinite(value)) {
                    value = input.min;
                }
                value = Math.max(
                    input.min,
                    Math.min(input.max, value)
                );
                value = Math.floor(value);
            }
            else {
                value = value.substring(0, input.max);
            }
            $(this).val(value);
        });
    }

    // =========================
    // SAVE SETTINGS
    // =========================

    $("#saveSettings").on("click", function () {
        const payload = {};
        for (const input of inputs) {
            if (input.id === "deviceId") continue;
            let value = $(`#${input.id}`).val();
            if (input.type === "number") {
                value = parseInt(value, 10);
            }
            payload[input.id] = value;
        }
        payload.deviceId = localStorage.getItem("deviceId");
        $.ajax({
            url: "/api/dash_info.php",
            method: "POST",
            timeout: 5000,
            contentType: "application/json",
            data: JSON.stringify(payload),
            success: function (res) {

                if (res.success) {
                    showMsg("Settings has been pushed!");
                } else {
                    showMsg("Failed to push settings.", "error");
                }
            },
            error: function (err) {

                console.error(err);
                showMsg("An error occurred while saving settings.", "error");
            }
        });
    });

    // =========================
    // MOTOR TOGGLE
    // =========================

    $("#motorStartBtn").on("click", function () {
        const $btn = $(this);
        $btn.prop("disabled", true);
        setTimeout(() => {
            $btn.prop("disabled", false);
        }, 10000);
        $.ajax({
            url: "/api/dash_info.php",
            method: "POST",
            timeout: 5000,
            contentType: "application/json",
            data: JSON.stringify({
                deviceId: localStorage.getItem("deviceId"),
                motorToggle: true
            }),
            success: function (res) {
                if (res.success) {
                    showMsg(res.message);
                    refreshPage();

                } else {
                    console.error(res.message);
                    showMsg(res.message, "error");
                }
            },
            error: function (err) {
                console.error(err);
                showMsg("An error occurred while toggling motor.", "error");
            }
        });
    });

    // =========================
    // UPDATE LAST SYNC TIMER
    // =========================

    setInterval(() => {
        if (!last_seen) return;
        $updateTimer.text(`last sync ${timeAgo(last_seen)}`);
        const seconds = Math.floor((Date.now() - last_seen.getTime()) / 1000);
        if (seconds > 20) {
            $statusDot.removeClass("good").addClass("danger");
            $sysStatus.text("System Offline");

        } else {
            $statusDot.removeClass("danger").addClass("good");
            $sysStatus.text("System Online");
        }
    }, 1000);

    // =========================
    // REFRESH PAGE
    // =========================

    async function refreshPage() {
        if (refreshBusy || document.hidden || !localStorage.getItem("deviceId")) return;
        refreshBusy = true;
        $.ajax({
            url: `/api/dash_info.php?id=${localStorage.getItem("deviceId")}`,
            method: "GET",
            timeout: 5000,
            success: function (res) {
                if (!res.success) {
                    setOfflineState();
                    return;
                }
                const data = res.data;
                last_seen = new Date(
                    data.last_updated.replace(" ", "T")
                );
                updateGauges(data);
                updateMotorState(data.motor);
            },
            error: async function (xhr) {
                // if (xhr.status === 401) {
                //     const refreshed = await refreshAccessToken();
                //     if (refreshed) {
                //         refreshPage();
                //         return;
                //     }
                // }
                $statusDot.removeClass("good").addClass("danger");
                $sysStatus.text("Connection Error");
            },
            complete: function () {
                refreshBusy = false;
            }
        });
    }

    // =========================
    // UPDATE GAUGES
    // =========================

    function updateGauges(data) {
        setGauge("waterArc", "waterText", data.level, data.level, "%");
        setGauge("voltageArc", "voltageText", toPercent(data.voltage), data.voltage, "V");
        setGauge("volumeArc", "volumeText", toPercent(data.level * 10, 0, 1000), data.level * 10, "L");
    }

    // =========================
    // UPDATE MOTOR STATE
    // =========================

    function updateMotorState(motor) {
        if (!motor) {
            $motorState
                .removeClass("good")
                .addClass("danger")
                .text("OFF")
                .data("state", "off");
        } else {
            $motorState
                .removeClass("danger")
                .addClass("good")
                .text("ON")
                .data("state", "on");
        }
    }

    // =========================
    // OFFLINE STATE
    // =========================

    function setOfflineState() {
        $statusDot.removeClass("good").addClass("danger");
        $sysStatus.text("System Offline");
    }

    // =========================
    // PERCENT CONVERTER
    // =========================

    function toPercent(value, min = 200, max = 270) {
        let percent = ((value - min) / (max - min)) * 100;
        percent = Math.max(0, Math.min(100, percent));
        return percent;
    }

    // =========================
    // TIME AGO
    // =========================

    function timeAgo(dateObj) {
        const now = new Date();
        const diffMs = now - dateObj;
        const diffSec = Math.floor(diffMs / 1000);
        const diffMin = Math.floor(diffSec / 60);
        const diffHour = Math.floor(diffMin / 60);
        const diffDay = Math.floor(diffHour / 24);
        const diffMonth = Math.floor(diffDay / 30);
        const diffYear = Math.floor(diffDay / 365);

        if (diffYear > 0) {
            return `${diffYear} year${diffYear > 1 ? "s" : ""} ago`;
        }

        if (diffMonth > 0) {
            return `${diffMonth} month${diffMonth > 1 ? "s" : ""} ago`;
        }

        if (diffDay > 0) {
            return `${diffDay} day${diffDay > 1 ? "s" : ""} ago`;
        }

        if (diffHour > 0) {
            return `${diffHour} hour${diffHour > 1 ? "s" : ""} ago`;
        }

        if (diffMin > 0) {
            return `${diffMin} min ago`;
        }

        return `${diffSec} sec ago`;
    }

    // =========================
    // NAVIGATION
    // =========================

    $(".nav button").on("click", function () {
        const page = $(this).data("page");
        if (!page) return;
        $(".page").addClass("hidden");
        $(".nav button").removeClass("active");
        $(this).addClass("active");
        $(`#page-${page}`).removeClass("hidden");
    });

    // =========================
    // MOBILE MENU
    // =========================

    $("#menu").on("click", function () {
        const $nav = $(".nav");
        if ($nav.is(":visible")) {
            $nav.slideUp(200);
        } else {
            $nav.slideDown(500, function () {
                $(this).css("display", "flex");
            });
        }
    });

    // =========================
    // PASSWORD TOGGLE
    // =========================

    $(".password-show").on("click", function () {
        const input = $("#wifiPassword");
        const type =
            input.attr("type") === "password"
                ? "text"
                : "password";

        input.attr("type", type);
        $(this).toggleClass("fa-eye fa-eye-slash");
    });

    // =========================
    // ADD DEVICE
    // =========================

    $("#addDeviceBtn").on("click", function () {
        const deviceId = $("#deviceId").val().trim();
        if (!deviceId || deviceId.length !== 12) {
            showMsg("Please enter a valid device ID.", "error");
            return;
        }

        $.ajax({
            url: "/api/add_device.php",
            method: "POST",
            timeout: 5000,
            contentType: "application/json",
            data: JSON.stringify({ deviceId }),
            success: function (res) {
                if (res.success) {
                    localStorage.setItem("deviceId", deviceId);
                    renderDevice(deviceId);
                    showMsg(res.message);
                } else {
                    showMsg(res.message, "error");
                }
            },
            error: function (err) {
                console.error(err);
                showMsg(
                    "An error occurred while adding the device.",
                    "error"
                );
            }
        });
    });

    // =========================
    // RENDER DEVICE
    // =========================

    function renderDevice(deviceId) {
        const formattedDeviceId = deviceId
            .toUpperCase()
            .replace(/(.{2})/g, "$1-")
            .slice(0, -1);

        $deviceList.empty().append(`
            <h3>DEVICE 1 (${formattedDeviceId})</h3>
            <button 
                class="device-btn danger remove-device"
                data-id="${deviceId}">
                Remove
            </button>
        `);
    }

    // =========================
    // LOAD DEVICE
    // =========================

    function loadDevice() {
        $.ajax({
            url: "/api/get_devices.php",
            method: "GET",
            timeout: 5000,
            success: function (res) {
                if (res.success && res.data) {
                    renderDevice(res.data.id);
                } else {
                    $deviceList.html(`
                        <p style="color:#95a8c7;">
                            No devices Found.
                            Please refresh the list or add a new device.
                        </p>
                    `);
                }
            },

            error: function (err) {
                console.error(err);
                $deviceList.html(`
                    <p style="color:#95a8c7;">
                        Failed to load devices.
                        Please try again later.
                    </p>
                `);
            }
        });
    }

    // =========================
    // REMOVE DEVICE
    // =========================

    $(document).on("click", ".remove-device", function () {
        localStorage.removeItem("deviceId");
        $deviceList.html(`
            <p style="color:#95a8c7;">
                No devices Found.
                Please refresh the list or add a new device.
            </p>
        `);

        showMsg("Device removed.");
    });

    // =========================
    // TOAST MESSAGE
    // =========================

    function showMsg(message, type = "success") {
        const toast = $(`
            <div class="toast ${type}">
                ${message}
            </div>
        `);
        $toastBox
            .append(toast)
            .css("display", "flex");

        setTimeout(() => {
            toast.fadeOut(300, function () {
                $(this).remove();
                if ($toastBox.children().length === 0) {
                    $toastBox.hide();
                }
            });

        }, 3000);
    }

    // =========================
    // INITIAL LOAD
    // =========================

    function setGauge(arcId, textId, percent, text, suffix = '%') {
        const $circle = $('#' + arcId);
        const $text = $('#' + textId);
        const circumference = 2 * Math.PI * 78;
        const offset = circumference - (percent / 100) * circumference;
        $circle.css({
            'stroke-dasharray': circumference.toFixed(2),
            'stroke-dashoffset': offset.toFixed(2)
        });

        $text.text(text + suffix);
    }

    function updateClock() {
        const now = new Date();
        $clockPill.text(
            now.toLocaleTimeString([], { hour12: false })
        );
    }


    async function refreshAccessToken() {
        try {
            await $.ajax({
                url: "/api/refresh.php",
                type: "POST",
            });
            return true;
        }
        catch (err) {
            window.location.href = "/auth";
            return false;
        }
    }

    async function checkAuth() {
        try {
            const response = await $.ajax({
                url: "/api/me.php",
                type: "GET",
            });
        }
        catch (err) {
            const refreshed = await refreshAccessToken();
            if (!refreshed) {
                window.location.href = "/auth";
            }
            else {
                location.reload();
            }
        }
    }

    await checkAuth();
    if (deviceId) {
        renderDevice(deviceId);
        refreshPage();
    } else {
        try {
            const response = await $.ajax({
                url: "/api/get_device.php",
                type: "GET"
            });
            if (response.success && response.device) {
                localStorage.setItem("deviceId", response.device);
            } else {
                $deviceList.html(`<p style="color:#95a8c7;">No devices Found.Please refresh the list or add a new device.</p>`);
            }
        } catch (err) {
            $deviceList.html(`<p style="color:#95a8c7;">No devices Found.Please refresh the list or add a new device.</p>`);
        }
    }
    updateClock();
    setInterval(updateClock, 1000);
    setInterval(refreshPage, 5000);
});