#include "web_dash.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
extern "C" {
#include "mbedtls/md.h"
}
#include "config.h"

const uint8_t NONCE_HISTORY = 10;
const uint8_t NONCE_LEN = 17;   // 16 chars + null
unsigned long lastRequestTime = 0;
char usedNonces[NONCE_HISTORY][NONCE_LEN];
int nonceIndex = 0;
const char* SECRET = "3e3c9fe7e5d099e1013e8f20c52b46ff0cea526d7bf472fc3195a928284300ce";

static AsyncWebServer server(80);
IPAddress trustedServer(10,174,113,67);

bool isReplay(const char *nonce) {
  for (int i = 0; i < NONCE_HISTORY; i++) {
    if (strcmp(usedNonces[i], nonce) == 0) {
      return true;
    }
  }

  strncpy(usedNonces[nonceIndex], nonce, NONCE_LEN);
  usedNonces[nonceIndex][NONCE_LEN - 1] = '\0';

  nonceIndex = (nonceIndex + 1) % NONCE_HISTORY;
  return false;
}

bool rateLimit() {
  unsigned long now = millis();
  if (now - lastRequestTime < 200) return false;  // 5 req/sec max
  lastRequestTime = now;
  return true;
}

String hmac_sha256(const String &data, const String &key) {
  byte hmac[32];

  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);

  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_setup(&ctx, info, 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char *)key.c_str(), key.length());
  mbedtls_md_hmac_update(&ctx, (const unsigned char *)data.c_str(), data.length());
  mbedtls_md_hmac_finish(&ctx, hmac);

  mbedtls_md_free(&ctx);

  char output[65];
  for (int i = 0; i < 32; i++) {
    sprintf(output + i * 2, "%02x", hmac[i]);
  }
  return String(output);
}

void handleStatusRequest(AsyncWebServerRequest *request) {
  char json[128];
  snprintf(json, sizeof(json), "{\"heap\":%u,\"version\":\"%s\"}", ESP.getFreeHeap(), VERSION);
  request->send(200, "application/json", json);
}

void web_dash_init() {

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    IPAddress ip = request->client()->remoteIP();
    Serial.print("Request from: ");
    Serial.println(ip);
    request->send(200, "text/html", "<h1>Web Dashboard</h1><br><p>Work in progress......</p>");

  });
  // server.on("/update", HTTP_POST, handleUpdate, handleUpload);
  server.on("/status", HTTP_GET, handleStatusRequest);

  server.on(
    "/api/7f3a9c_motor_ctrl", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t) {
      // 🔐 IP Restriction
      if (request->client()->remoteIP() != trustedServer) {
        request->send(403, "text/plain", "Forbidden");
        return;
      }

      // ⚡ Rate limiting
      if (!rateLimit()) {
        request->send(429, "text/plain", "Too Many Requests");
        return;
      }

      // Parse JSON
      DynamicJsonDocument doc(512);
      deserializeJson(doc, data, len);

      String action = doc["action"];
      long timestamp = doc["timestamp"];
      const char* nonce = doc["nonce"];
      String sig = doc["sig"];

      // 🔁 Replay protection
      if (isReplay(nonce)) {
        request->send(403, "text/plain", "Replay detected");
        return;
      }

      // ⏱ Timestamp check (±30 sec)
      long now = time(NULL);
      if (abs(now - timestamp) > 30) {
        Serial.println(now);
        Serial.println(timestamp);
        request->send(403, "text/plain", "Expired request");
        return;
      }

      // Rebuild payload
      doc.remove("sig");
      char payload[128];
      snprintf(payload, sizeof(payload), "{\"action\":\"%s\",\"timestamp\":%ld,\"nonce\":\"%s\"}", action.c_str(), timestamp, nonce);

      String expected = hmac_sha256(payload, SECRET);

      if (expected != sig) {
        request->send(403, "text/plain", "Invalid signature");
        return;
      }

      // ✅ Strict action validation
      if (action == "motor_on") {
        digitalWrite(15, HIGH);
      } else if (action == "motor_off") {
        digitalWrite(15, LOW);
      } else {
        request->send(400, "text/plain", "Invalid action");
        return;
      }

      request->send(200, "text/plain", "OK");
    });
  server.begin();
}
