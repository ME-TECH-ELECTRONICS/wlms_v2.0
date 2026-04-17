const char page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 OTA Update</title>
<script src="https://code.jquery.com/jquery-3.6.0.min.js"></script>
<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.0/css/all.min.css">
<style>
body {font-family: 'Segoe UI', sans-serif;background: linear-gradient(135deg, #1e3c72, #2a5298);color: #fff;text-align: center;}
.container {max-width: 600px;margin: 50px auto;background: rgba(0,0,0,0.3);padding: 30px;border-radius: 15px;box-shadow: 0 10px 30px rgba(0,0,0,0.5);}
h1 {margin-bottom: 10px;}
.status {margin: 20px 0;font-size: 14px;text-align: left;}
.status div {margin: 5px 0;}
.file-input {margin: 20px 0;}
input[type=file] {padding: 10px;background: #fff;color: #000;border-radius: 5px;}
button {background: #00c6ff;border: none;padding: 12px 25px;color: #fff;border-radius: 5px;cursor: pointer;font-size: 16px;}
button:hover {background: #0072ff;}
.progress-bar {width: 100%;background: #333;border-radius: 10px;overflow: hidden;margin-top: 20px;}
.progress {height: 20px;width: 0%;background: #00ffcc;transition: width 0.3s;}
#percent {margin-top: 5px;font-size: 14px;}
.log {margin-top: 20px;text-align: left;font-size: 12px;background: rgba(0,0,0,0.4);padding: 10px;border-radius: 5px;height: 120px;overflow-y: auto;}
.valid {color: #00ffcc;}
.invalid {color: #ff4d4d;}
</style>
</head>
<body>
<div class="container">
    <h1><i class="fas fa-microchip"></i> ESP32 OTA Update</h1>
    <p>Upload firmware and update your device in real-time</p>
    <div class="status">
        <div><i class="fas fa-microchip"></i> Chip: <span id="chip">ESP32</span></div>
        <div><i class="fas fa-memory"></i> Free Heap: <span id="heap">--</span></div>
        <div><i class="fas fa-code-branch"></i> Firmware: <span id="version">--</span></div>
    </div>
    <div class="file-input">
        <input type="file" id="firmware">
        <div id="validation"></div>
    </div>
    <button id="uploadBtn"><i class="fas fa-upload"></i> Upload</button>
    <div class="progress-bar">
        <div class="progress" id="progress"></div>
    </div>
    <div id="percent">0%</div>
    <div class="log" id="log"></div>
</div>
<script>
function log(msg) {$('#log').append(msg + "<br>");$('#log').scrollTop($('#log')[0].scrollHeight);}
function fetchStatus() {$.get('/status', function(data) {$('#heap').text(data.heap || 'N/A');$('#version').text(data.version || 'N/A');}, 'json').fail(function() {log("Status fetch failed");});}
setInterval(fetchStatus, 2000);
$('#firmware').on('change', function() {let file = this.files[0];if (!file) return;if (!file.name.endsWith('.bin')) {$('#validation').html("<span class='invalid'>Invalid file type. Only .bin allowed</span>");return;}if (file.size > 4 * 1024 * 1024) {$('#validation').html("<span class='invalid'>File too large</span>");return;}$('#validation').html("<span class='valid'>File looks good</span>");});
$('#uploadBtn').click(function() {let file = $('#firmware')[0].files[0];if (!file) {log("No file selected");return;}$('#uploadBtn').prop('disabled', true);let formData = new FormData();formData.append('update', file);
    $.ajax({url: '/update?key=' + localStorage.getItem('ota_key'),type: 'POST',data: formData,contentType: false,processData: false,
        xhr: function() {let xhr = new XMLHttpRequest();xhr.upload.addEventListener("progress", function(evt) {if (evt.lengthComputable) {let percent = (evt.loaded / evt.total) * 100;$('#progress').css('width', percent + '%');$('#percent').text(Math.round(percent) + '%');}}, false);return xhr;},
        success: function() {log("Upload complete. Rebooting...");},
        error: function() {log("Upload failed");$('#uploadBtn').prop('disabled', false);}
    });
});
</script>
</body>
</html>
)rawliteral";