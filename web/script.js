$(document).ready(function() {
    var r = $("#readings");
    var c = $(".toggle-btn").children("span");
    $(".menu span").click(function() {
        var c = $(this).text().toLowerCase();
        $("#contents").children("div").hide();
        $(`.${c}`).fadeIn(500);
    });
    $(".toggle-btn input").change(updateText)
    $("#status").css({
        color: "red"
    });
    $("#ctrl").css({
        color: "#5CB85C"
    });
    updateText();
    r.append(createGuage(100, 30, "Water Level", "%"));
    r.append(createGuage(1000, 300, "Volume", "L"));
    r.append(createGuage(250, 230, "Line Volume", "V"));
    r.append(createGuage(30, 12, "Last Motor ON Time", "Min", true));
    r.append(createGuage(5, 2, "Daily Trigger Rate", "", true));
    function sendCmd(cmd) {}
    function updateText() {
        if ($('#mode').is(':checked')) {
            c.eq(1).css({
                "font-weight": "bold"
            });
            c.eq(0).css({
                "font-weight": "300"
            });
            $("#motorCtrl").hide();
        } else {
            c.eq(0).css({
                "font-weight": "bold"
            });
            c.eq(1).css({
                "font-weight": "300"
            });
            $("#motorCtrl").css({
                "display": "flex"
            });
        }
        if ($('#ctrlBtn').is(':checked')) {
            c.eq(3).css({
                "font-weight": "bold"
            });
            c.eq(2).css({
                "font-weight": "300"
            });
        } else {
            c.eq(2).css({
                "font-weight": "bold"
            });
            c.eq(3).css({
                "font-weight": "300"
            });
        }
    }
    function csvToJson(csv) {
        const lines = csv.split('\n');
        const headers = lines[0].split(',');

        const result = [];
        for (let i = 1; i < lines.length; i++) {
            const obj = {};
            const currentline = lines[i].split(',');
            for (let j = 0; j < headers.length; j++) {
                obj[headers[j].trim()] = currentline[j].trim();
            }
            result.push(obj);
        }
        return result;
    }
});

function createGuage(max, val, head, suffix, invert = false) {
    const percentage = val / max;
    var color;
    var deg = `rotate(${percentage * 180}deg)`;
    const Red = "red";
    const Green = "#5CB85C";
    const Yellow = "#FFEA00";
    if (percentage*100 < 40) {
        color = Red;
        if (invert) color = Green;
    } else if (percentage*100 < 80 & percentage*100 >= 40) {
        color = Yellow;
    } else {
        color = Green;
        if (invert) color = Red
    }
    var item = $("<div>",
        {
            class: "items"
        });
    var heading = $("<span>",
        {
            text: head
        });

    var mainDiv = $("<div>",
        {
            class: "gauge",
            css: {
                "background-color": color
            }
        });
    var percent = $("<div>",
        {
            class: "percentage",
            css: {
                "transform": deg
            }
        });
    var mask = $("<div>",
        {
            class: "mask"
        });
    var value = $("<span>",
        {
            class: "value",
            text: val + suffix
        });
    item.append(heading,
        mainDiv.append(percent, mask, value));
    return item;
}

function createPlot(data) {
    Plotly.newPlot('plot',
        data,
        {},
        {
            displaylogo: false
        });
}
trace1 = {
    type: 'scatter',
    x: [1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10],
    y: [10,
        15,
        13,
        17,
        24,
        12,
        26,
        30,
        25,
        33],
    mode: 'lines',
    name: 'Green',
    line: {
        color: 'rgb(24, 200, 20)',
        width: 3
    }
};
trace2 = {
    type: 'scatter',
    x: [0,
        2,
        4,
        6,
        8,
        10,
        12,
        14,
        16,
        18],
    y: [10,
        15,
        13,
        17,
        24,
        12,
        26,
        30,
        25,
        33],
    mode: 'lines',
    name: 'Red',
    line: {
        color: 'rgb(219, 64, 82)',
        width: 3
    }
};

var data = [trace1, trace2];
createPlot(data)