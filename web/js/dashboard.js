$(document).ready(function () {

    // =========================================================
    // STATE
    // =========================================================

    let deviceId = localStorage.getItem("deviceId");
    let lastSeen = null;
    let refreshBusy = false;
    let refreshPromise = null;
    let logNewestFirst = true;

    const GAUGE_CIRCUMFERENCE = 2 * Math.PI * 78;
    const GAUGE_CIRCUMFERENCE_FIXED = GAUGE_CIRCUMFERENCE.toFixed(2);
    // =========================================================
    // DOM CACHE
    // =========================================================

    const $motorState = $("#motorState");
    const $statusDot = $(".status-dot");
    const $sysStatus = $("#sysStatus");
    const $updateTimer = $("#updateTimer");
    const $deviceList = $("#deviceList");
    const $toastBox = $("#toastBox");
    const $clockPill = $("#clockPill");
    const $logBody = $("#logBody");
    const $logSearch = $("#logSearch");
    const $logFilter = $("#logFilter");
    const $sortBtn = $("#sortBtn");

    // =========================================================
    // CONSTANTS
    // =========================================================

    const NO_DEVICE_HTML = `<p style="color:#95a8c7;">No devices Found. Please refresh the list or add a new device.</p>`;
    const EVENT_TYPES = {
        0: "Normal",
        1: "Fault",
        2: "Power",
        3: "Manual",
        4: "Settings",
    };
    const EVENT_REASONS = {
        1: "Motor turned on. Normal operation.",
        2: "Motor turned off. Normal operation.",
        3: "Motor turned off. Tank full detected.",
        4: "Motor stopped. Low voltage detected.",
        5: "Motor stopped. Dry run detected.",
        6: "Sensor communication lost.",
        7: "Maximum motor runtime exceeded.",
        8: "Power restored. System resumed operation.",
        9: "Power outage detected. Motor stopped.",
        10: "Controller settings updated.",
        11: "Motor turned on manually.",
        12: "Motor turned off manually.",
        13: "System mode changed to manual.",
        14: "System mode changed to automatic.",
    };
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

    const gauges = {
        water: {
            $arc: $("#waterArc"),
            $text: $("#waterText")
        },
        voltage: {
            $arc: $("#voltageArc"),
            $text: $("#voltageText")
        },
        volume: {
            $arc: $("#volumeArc"),
            $text: $("#volumeText")
        }
    };

    // =========================================================
    // INITIALIZATION
    // =========================================================

    init().catch(err => {
        console.error("Initialization failed:", err);
    });

    async function init() {

        bindInputValidation();
        bindEvents();

        await checkAuth();

        if (deviceId) {
            renderDevice(deviceId);
            await refreshPage();
        } else {
            await loadDevice();
        }

        updateClock();
        initializeLogs();
        setInterval(updateClock, 1000);
        setInterval(updateLastSeenTimer, 1000);
        setInterval(refreshPage, 5000);
    }

    // =========================================================
    // EVENT BINDINGS
    // =========================================================

    function bindEvents() {

        $("#saveSettings").on("click", saveSettings);
        $("#motorStartBtn").on("click", toggleMotor);
        $("#menu").on("click", toggleMobileMenu);
        $("#deviceRefresh").on("click", loadDevice);
        $("#logout").on("click", logout);
        $(".password-show").on("click", togglePasswordVisibility);
        $("#addDeviceBtn").on("click", addDevice);
        $(".nav button").on("click", switchPage);
        $(document).on("click", ".remove-device", removeDevice);
    }

    // =========================================================
    // INPUT VALIDATION
    // =========================================================

    function bindInputValidation() {
        for (const input of inputs) {
            $(`#${input.id}`).on("input", function () {
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
                    value = Math.floor(
                        Math.max(input.min, Math.min(input.max, value))
                    );
                } else {
                    value = value.substring(0, input.max);
                }
                $(this).val(value);
            });
        }
    }

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

    function togglePasswordVisibility(event) {
        const $btn = $(event.currentTarget);
        const $input = $("#wifiPassword");
        const type = $input.attr("type") === "password" ? "text" : "password";
        $input.attr("type", type);
        $btn.toggleClass("fa-eye fa-eye-slash");
    }

    function switchPage() {
        const page = $(this).data("page");
        if (!page) return;
        $(".page").addClass("hidden");
        $(".nav button").removeClass("active");
        $(this).addClass("active");
        $(`#page-${page}`).removeClass("hidden");
    }

    function toggleMobileMenu() {
        const $nav = $(".nav");

        $nav.stop(true, true).slideToggle(200, function () {
            if ($nav.is(":visible")) {
                $nav.css("display", "flex");
            }
        });
    }

    function removeDevice() {
        localStorage.removeItem("deviceId");
        $deviceList.html(NO_DEVICE_HTML);
        deviceId = null;
        lastSeen = null;
        setStatus(false);
        updateMotorState(false);
        showMsg("Device removed.");
    }

    async function logout() {
        try {
            const res = await apiRequest({
                url: "/api/logout.php",
                method: "POST"
            });
            if (res.success) {
                localStorage.removeItem("deviceId");
                location.href = "/auth";

            } else {
                showMsg("Logout failed: " + res.message, "error");
            }
        } catch (err) {
            console.error(err);
        }
    }

    async function addDevice() {
        const newDeviceId = $("#deviceId").val().trim();
        if (!newDeviceId || newDeviceId.length !== 12) {
            showMsg("Please enter a valid device ID.", "error");
            return;
        }
        try {
            const res = await apiRequest({
                url: "/api/add_device.php",
                method: "POST",
                data: { deviceId: newDeviceId }
            });
            if (res.success) {
                localStorage.setItem("deviceId", newDeviceId);
                deviceId = newDeviceId;
                renderDevice(deviceId);
                showMsg(res.message);
            } else {
                showMsg(res.message, "error");
            }
        } catch (err) {
            console.error(err);
            showMsg("An error occurred while adding the device.", "error");
        }
    }

    async function loadDevice() {
        try {
            const res = await apiRequest({
                url: "/api/get_device.php"
            });
            if (res.success && res.device) {
                deviceId = res.device;
                localStorage.setItem("deviceId", deviceId);
                renderDevice(deviceId);
                await refreshPage();
            } else {
                $deviceList.html(NO_DEVICE_HTML);
            }
            showMsg("Device list updated.");
        } catch (err) {
            console.error(err);
            $deviceList.html(NO_DEVICE_HTML);
        }
    }

    async function saveSettings() {
        const payload = {};
        for (const input of inputs) {
            if (input.id === "deviceId") continue;
            let value = $(`#${input.id}`).val();
            if (input.type === "number") {
                value = value === "" ? null : parseInt(value, 10);
            }
            payload[input.id] = value;
        }
        payload.deviceId = deviceId;
        try {
            const res = await apiRequest({
                url: "/api/dash_info.php",
                method: "POST",
                data: payload
            });
            if (res.success) {
                showMsg(res.message);
            } else {
                showMsg(res.message, "error");
            }
        } catch (err) {
            console.error(err);
            showMsg("An error occurred while saving settings.", "error");
        }
    }

    async function toggleMotor() {
        const $btn = $("#motorStartBtn");
        $btn.prop("disabled", true);
        try {
            const res = await apiRequest({
                url: "/api/dash_info.php",
                method: "POST",
                data: {
                    deviceId,
                    motorToggle: true
                }
            });
            if (res.success) {
                showMsg(res.message);
                await refreshPage();
            } else {
                console.error(res.message);
                showMsg(res.message, "error");
            }
        } catch (err) {
            console.error(err);
            showMsg("An error occurred while toggling motor.", "error");
        } finally {
            setTimeout(() => {
                $btn.prop("disabled", false);
            }, 10000);
        }
    }

    // =========================================================
    // API FUNCTIONS
    // =========================================================

    async function refreshPage() {
        if (refreshBusy || document.hidden || !deviceId) {
            return;
        }
        refreshBusy = true;
        try {
            const res = await apiRequest({
                url: `/api/dash_info.php?id=${deviceId}`,
                method: "GET",
            });
            if (!res?.success || !res?.data) {
                setStatus(false);
                return;
            }
            const data = res.data;
            if (typeof data.last_updated === "string") {
                const parsedDate = new Date(
                    data.last_updated.replace(" ", "T")
                );

                if (!isNaN(parsedDate)) {
                    lastSeen = parsedDate;
                }
            }
            updateGauges(data);
            updateMotorState(data.motor);
        } catch (err) {
            console.error(err);
            setConnectionError();
        } finally {
            refreshBusy = false;
        }
    }

    async function refreshAccessToken() {
        if (refreshPromise) {
            return refreshPromise;
        }
        refreshPromise = (async () => {
            try {
                await $.ajax({
                    url: "/api/refresh.php",
                    type: "POST"
                });
                return true;
            } catch {
                window.location.href = "/auth";
                return false;
            } finally {
                refreshPromise = null;
            }
        })();
        return refreshPromise;
    }

    async function checkAuth() {
        try {
            await apiRequest({ url: "/api/me.php" });
        } catch {
            window.location.href = "/auth";
        }
    }

    // =========================================================
    // UI FUNCTIONS
    // =========================================================

    function setStatus(isOnline) {
        $statusDot
            .toggleClass("good", isOnline)
            .toggleClass("danger", !isOnline);

        $sysStatus.text(
            isOnline
                ? "Device Online"
                : "Device Offline"
        );
    }

    function setConnectionError() {
        $statusDot.removeClass("good").addClass("danger");
        $sysStatus.text("Connection Error");
    }

    function updateMotorState(motor) {
        const isOn = Boolean(motor);
        $motorState
            .toggleClass("good", isOn)
            .toggleClass("danger", !isOn)
            .text(isOn ? "ON" : "OFF")
            .data("state", isOn ? "on" : "off");
    }

    function updateGauges(data) {
        setGauge(
            gauges.water,
            data.level,
            data.level,
            "%"
        );
        setGauge(
            gauges.voltage,
            toPercent(data.voltage),
            data.voltage,
            "V"
        );
        setGauge(
            gauges.volume,
            toPercent(data.level * 10, 0, 1000),
            data.level * 10,
            "L"
        );
    }

    function setGauge(gauge, percent, text, suffix = "%") {
        const offset = GAUGE_CIRCUMFERENCE - (percent / 100) * GAUGE_CIRCUMFERENCE;
        gauge.$arc.css({
            strokeDasharray: GAUGE_CIRCUMFERENCE_FIXED,
            strokeDashoffset: offset.toFixed(2)
        });
        gauge.$text.text(text + suffix);
    }

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

    function renderLogs(logs) {
        const query = ($logSearch.val() || "").toLowerCase();
        const filter = $logFilter.val();
        const filteredLogs = logs.filter(item => {
            const searchText = [
                item.date ?? "",
                item.type ?? "",
                item.desc ?? "",
                item.status ?? ""
            ].join(" ").toLowerCase();
            const matchesQuery = searchText.includes(query);
            const matchesFilter = filter === "all" || (item.type ?? "").toLowerCase() === filter.toLowerCase();
            return matchesQuery && matchesFilter;
        }).sort((a, b) => {
            const dateA = a.timestamp ?? new Date((a.date ?? "").replace(" ", "T")).getTime();
            const dateB = b.timestamp ?? new Date((b.date ?? "").replace(" ", "T")).getTime();
            return logNewestFirst ? dateB - dateA : dateA - dateB;
        });
        $logBody.empty();
        if (!filteredLogs.length) {
            $logBody.append(
                $("<tr>").append(
                    $("<td>")
                        .attr("colspan", 4)
                        .css("color", "#95a8c7")
                        .text("No log entries found.")
                )
            );
            return;
        }
        const rows = [];
        for (const item of filteredLogs) {
            const statusClass = item.status === "Success" ? "good" : item.status === "Resolved" ? "neutral" : item.status === "Blocked" ? "danger" : "warn";
            rows.push(
                $("<tr>")
                    .append($("<td>").text(item.date ?? ""))
                    .append($("<td>").text(item.type ?? ""))
                    .append($("<td>").text(item.desc ?? ""))
                    .append(
                        $("<td>").append(
                            $("<span>")
                                .addClass(`badge ${statusClass}`)
                                .text(item.status ?? "")
                        )
                    )
            );
        }
        $logBody.append(rows);
    }

    async function initializeLogs() {
        try {
            const res = await apiRequest({
                url: `/api/logs.php?id=${deviceId}`,
                method: "GET",
            });
            let logs = res.logs || [];
            logs.forEach(log => {
                log.timestamp = new Date(
                    (log.date ?? "").replace(" ", "T")
                ).getTime();
            });
            renderLogs(logs);
            $logSearch.on("input", () => renderLogs(logs));
            $logFilter.on("change", () => renderLogs(logs));
            $sortBtn.on("click", function () {
                logNewestFirst = !logNewestFirst;
                $(this).text(
                    `Sort: ${logNewestFirst ? "Newest" : "Oldest"}`
                );
                renderLogs(logs);
            });
        } catch (error) {
            console.error("Error fetching logs:", error);
        }

    }

    // =========================================================
    // UTILITIES
    // =========================================================

    function toPercent(value, min = 200, max = 270) {
        return Math.max(
            0,
            Math.min(
                100,
                ((value - min) / (max - min)) * 100
            )
        );
    }

    function updateClock() {
        if (document.hidden) {
            return;
        }
        $clockPill.text(
            new Date().toLocaleTimeString([], {
                hour12: false
            })
        );
    }

    function updateLastSeenTimer() {
        if (!lastSeen) {
            return;
        }
        $updateTimer.text(
            `last sync ${timeAgo(lastSeen)}`
        );
        const seconds = Math.floor(
            (Date.now() - lastSeen.getTime()) / 1000
        );
        setStatus(seconds <= 20);
    }

    function timeAgo(dateObj) {
        const diffSec = Math.floor((Date.now() - dateObj) / 1000);
        if (diffSec < 60) {
            return `${diffSec} sec${diffSec !== 1 ? "s" : ""} ago`;
        }

        const diffMin = Math.floor(diffSec / 60);
        if (diffMin < 60) {
            return `${diffMin} min${diffMin !== 1 ? "s" : ""} ago`;
        }

        const diffHour = Math.floor(diffMin / 60);
        if (diffHour < 24) {
            return `${diffHour} hour${diffHour !== 1 ? "s" : ""} ago`;
        }

        const diffDay = Math.floor(diffHour / 24);
        return `${diffDay} day${diffDay !== 1 ? "s" : ""} ago`;
    }
    function showMsg(message, type = 'success') {
        console.log($("#toastBox").length);
        let toast = $(`<div class="toast ${type}">${message}</div>`);
        $("#toastBox").append(toast);
        setTimeout(() => {
            toast.fadeOut(300, function () {
                $(this).remove();
            });
        }, 3000);
    }
    function decodeEvent(event) {
        const type = (event >> 5) & 0x07;
        const reason = event & 0x1F;
        return {
            type: EVENT_TYPES[type] || "Unknown",
            desc: EVENT_REASONS[reason] || "Unknown event"
        };
    }
});
