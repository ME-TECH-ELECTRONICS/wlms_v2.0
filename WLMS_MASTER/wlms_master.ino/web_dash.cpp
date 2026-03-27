#include "web_dash.h"

extern "C" {
#include "esp_ota_ops.h"
#include "esp_partition.h"
}

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include "mbedtls/base64.h"     // For decoding Basic Auth
#include "mbedtls/md.h"  // For SHA256
#include "index.html.h"
#include "config.h"

struct TempKey {
  String key;
  unsigned long expiry; // millis() when it expires
};
TempKey activeKey;

static AsyncWebServer server(80);

static volatile size_t ota_received = 0;
static volatile size_t ota_total = 0;
static volatile bool ota_success = false;

static String ota_log = "";

String sha256(String input) {
  unsigned char hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char*)input.c_str(), input.length());
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  char buf[65];
  for (int i = 0; i < 32; i++) sprintf(buf + i * 2, "%02x", hash[i]);
  buf[64] = 0;
  return String(buf);
}

String base64Decode(String input) {
    size_t len = input.length();
    size_t outputLen = 0;
    unsigned char decoded[len];
    mbedtls_base64_decode(decoded, len, &outputLen,
                          (const unsigned char*)input.c_str(), len);
    return String((char*)decoded).substring(0, outputLen);
}

bool checkAuth(AsyncWebServerRequest *request) {
  if (!request->hasHeader("Authorization")) return false;
  const AsyncWebHeader* h = request->getHeader("Authorization");
  String auth = h->value(); // e.g., "Basic xxxx"
  if (!auth.startsWith("Basic ")) return false;
  String encoded = auth.substring(6);
  String decoded = base64Decode(encoded); // user:pass
  Serial.println(encoded);
  Serial.println(decoded);
  int sep = decoded.indexOf(':');
  if (sep < 0) return false;
  String user = decoded.substring(0, sep);
  String pass = decoded.substring(sep + 1);
  return (user == WEB_DASH_USERNAME && pass == WEB_DASH_PASSWORD);
}

// ---------------- Generate Key ----------------
void handleGenerateKey(AsyncWebServerRequest *request) {
  if (!checkAuth(request)) {
    request->requestAuthentication();
    return;
  }
  unsigned long now = millis();
  unsigned long expiry = now + 3600UL * 1000UL; // 1 hour
  String raw = String(WEB_DASH_USERNAME) + WEB_DASH_PASSWORD + WEB_DASH_SECRET + String(expiry);
  String key = sha256(raw);

  activeKey.key = key;
  activeKey.expiry = expiry;

  String json = "{\"key\":\"" + key + "\",\"expiry\":" + String(expiry) + "}";
  request->send(200, "application/json", json);
}

// ---------------- Validate Key ----------------
bool validateKey(String key) {
  unsigned long now = millis();
  return (key == activeKey.key && now < activeKey.expiry);
}

// ---------------- OTA Page ----------------
void handleUpdaterPage(AsyncWebServerRequest *request) {
  if (!checkAuth(request)) {
    request->requestAuthentication();
    return;
  }
  request->send(200, "text/html", page);
}

void handleStatusRequest(AsyncWebServerRequest *request) {
  if (!request->hasParam("key")) { 
    request->send(403, "application/json", "{\"error\":\"missing_key\"}"); 
    return; 
  }
  String key = request->getParam("key")->value();
  if (!validateKey(key)) { 
    request->send(403, "application/json", "{\"error\":\"invalid_key\"}"); 
    return; 
  }
  char json[128];
  snprintf(json, sizeof(json),
         "{\"wifi\":\"%s\",\"heap\":%u,\"version\":\"1.0.0\"}",
         WiFi.SSID().c_str(),
         ESP.getFreeHeap());
  request->send(200, "application/json", json);
}

// ---------------- OTA Upload ----------------
// void handleUpdate(AsyncWebServerRequest *request) {
//   if (!request->hasParam("key", true)) { request->send(403, "text/plain", "Forbidden"); return; }
//   String key = request->getParam("key", true)->value();
//   if (!validateKey(key)) { request->send(403, "text/plain", "Forbidden"); return; }

//   // OTA handled in onUpload callback below
//   request->send(200, "text/plain", "Upload started");
// }

// =========================
// OTA Upload
// =========================
// static void handleUpdateUpload() {
//   HTTPUpload& up = server.upload();

//   if (up.status == UPLOAD_FILE_START) {

//     ota_log = "Starting OTA...";
//     ota_received = 0;
//     ota_total = up.totalSize;

//     const esp_partition_t* part = esp_ota_get_next_update_partition(NULL);

//     if (!Update.begin(part->size)) {
//       ota_log = "Begin Failed";
//       return;
//     }

//   } else if (up.status == UPLOAD_FILE_WRITE) {

//     if (Update.write(up.buf, up.currentSize) != up.currentSize) {
//       ota_log = "Write Failed";
//       return;
//     }

//     ota_received += up.currentSize;

//   } else if (up.status == UPLOAD_FILE_END) {

//     if (Update.end(true)) {
//       ota_log = "Success. Rebooting...";
//       ota_success = true;
//     } else {
//       ota_log = "Failed";
//     }
//   }
// }

// =========================
// Setup Server
// =========================
void web_dash_init() {

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "<h1>Web Dashboard</h1><br><p>Work in progress......</p>");
  });
  server.on("/updater", HTTP_GET, handleUpdaterPage);
  server.on("/status", HTTP_GET, handleStatusRequest);

  // server.on(
  //   "/update", HTTP_POST, []() {
  //     if (!auth()) return;

  //     server.send(200, "text/plain",
  //                 ota_success ? "Success" : "Failed");

  //     delay(1000);
  //     if (ota_success) ESP.restart();
  //   },
  //   handleUpdateUpload);

  server.begin();
}
