$(document).ready(function () {

    const $logBody = $("#logBody");
    const $logSearch = $("#logSearch");
    const $logFilter = $("#logFilter");
    const $sortBtn = $("#sortBtn");

    let logNewestFirst = true;

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

    function decodeEvent(event) {
        const type = (event >> 5) & 0x07;
        const reason = event & 0x1F;
        return {
            type: EVENT_TYPES[type] || "Unknown",
            desc: EVENT_REASONS[reason] || "Unknown event"
        };
    }
    console.log(decodeEvent(0b01101011)); // Example usage
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

    function initializeLogs(logs) {
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
    }

    const logs = [
        { date: '2026-04-17 08:10:21', type: 'Motor', desc: 'Motor started automatically at low tank level.', status: 'Success' },
        { date: '2026-04-17 08:22:49', type: 'Alarm', desc: 'Voltage dip detected; controller kept motor running after recovery.', status: 'Resolved' },
        { date: '2026-04-17 08:35:12', type: 'Settings', desc: 'Start threshold changed from 30% to 35%.', status: 'Success' },
        { date: '2026-04-17 09:01:05', type: 'Security', desc: 'Failed login attempt blocked from unknown IP.', status: 'Blocked' },
        { date: '2026-04-16 19:45:33', type: 'Motor', desc: 'Motor stopped after tank reached upper threshold.', status: 'Success' },
        { date: '2026-04-16 13:19:10', type: 'Alarm', desc: 'Overcurrent warning triggered for 8 seconds.', status: 'Resolved' },
        { date: '2026-04-15 22:07:43', type: 'Settings', desc: 'Motor timeout set to 30 minutes.', status: 'Success' },
        { date: '2026-04-15 18:50:02', type: 'Security', desc: 'Admin session created successfully.', status: 'Success' }
    ];

    initializeLogs(logs);

});