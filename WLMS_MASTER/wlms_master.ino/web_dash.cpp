#include "web_dash.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
extern "C" {
#include "mbedtls/md.h"
}
#include "config.h"
#include "fsm_controller.h"

const uint8_t NONCE_HISTORY = 10;
uint32_t nonceTable[NONCE_HISTORY];

const uint8_t NONCE_LEN = 17;  // 16 chars + null
unsigned long lastRequestTime = 0;

int nonceIndex = 0;

static AsyncWebServer server(80);

uint32_t simpleHash(const char *str) {
  uint32_t hash = 5381;
  while (*str) {
    hash = ((hash << 5) + hash) ^ *str++;  // djb2 variant
  }
  return hash;
}

bool isReplay(const char *nonce) {

  uint32_t h = simpleHash(nonce);
  int index = h % NONCE_HISTORY;

  // Check if already used
  if (nonceTable[index] == h) {
    return true;
  }

  // Store new nonce
  nonceTable[index] = h;

  return false;
}

bool rateLimit() {
  unsigned long now = millis();
  if (now - lastRequestTime < 200) return false;  // 5 req/sec max
  lastRequestTime = now;
  return true;
}

bool constantTimeCompare(const char *a, const char *b, size_t len) {
  uint8_t result = 0;
  for (size_t i = 0; i < len; i++) {
    result |= a[i] ^ b[i];
  }
  return result == 0;
}

void hmac_sha256(const char *data, const char *key, char *output_hex) {
  uint8_t hmac[32];

  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);

  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_setup(&ctx, info, 1);

  mbedtls_md_hmac_starts(&ctx, (const unsigned char *)key, strlen(key));
  mbedtls_md_hmac_update(&ctx, (const unsigned char *)data, strlen(data));
  mbedtls_md_hmac_finish(&ctx, hmac);

  mbedtls_md_free(&ctx);

  // Convert to hex
  const char hex[] = "0123456789abcdef";
  for (int i = 0; i < 32; i++) {
    output_hex[i * 2] = hex[(hmac[i] >> 4) & 0xF];
    output_hex[i * 2 + 1] = hex[hmac[i] & 0xF];
  }
  output_hex[64] = '\0';
}

void handleStatusRequest(AsyncWebServerRequest *request) {
  char json[128];
  snprintf(json, sizeof(json), "{\"heap\":%u,\"version\":\"%s\"}", ESP.getFreeHeap(), VERSION);
  request->send(200, "application/json", json);
}

void handleSystemCtrl(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t) {

  // ⚡ Rate limiting
  if (!rateLimit()) {
    request->send(429, "text/plain", F("Too Many Requests"));
    return;
  }

  // ---- Size check ----
  if (len < 50 || len > 200) {
    request->send(400, "text/plain", F("Invalid payload size"));
    return;
  }

  // ---- Copy safely ----
  char buf[201];
  memcpy(buf, data, len);
  buf[len] = '\0';

  // ---- Extract fields ----
  char action[16] = { 0 };
  char nonce[NONCE_LEN] = { 0 };
  char sig[65] = { 0 };
  long timestamp = 0;

  char *p;

  // action
  p = strstr(buf, "\"action\":\"");
  if (!p || sscanf(p + 10, "%15[^\"]", action) != 1) {
    request->send(400, "text/plain", F("Invalid action"));
    return;
  }

  // timestamp
  p = strstr(buf, "\"timestamp\":");
  if (!p || sscanf(p + 12, "%ld", &timestamp) != 1) {
    request->send(400, "text/plain", F("Invalid timestamp"));
    return;
  }

  // nonce
  p = strstr(buf, "\"nonce\":\"");
  if (!p || sscanf(p + 9, "%16[^\"]", nonce) != 1) {
    request->send(400, "text/plain", F("Invalid nonce"));
    return;
  }

  // sig
  p = strstr(buf, "\"sig\":\"");
  if (!p || sscanf(p + 7, "%64[^\"]", sig) != 1) {
    request->send(400, "text/plain", F("Invalid signature"));
    return;
  }

  // ---- Strict validation ----
  if (nonce[16] != '\0' || sig[64] != '\0') {
    request->send(400, "text/plain", F("Invalid field length"));
    return;
  }

  // ---- Replay protection ----
  if (isReplay(nonce)) {
    request->send(403, "text/plain", F("Replay detected"));
    return;
  }

  // ---- Timestamp check ----
  long now = time(NULL);
  if (now < 100000) {
    request->send(503, "text/plain", F("Time not synced"));
    return;
  }

  if (abs(now - timestamp) > 30) {
    request->send(403, "text/plain", F("Expired request"));
    return;
  }

  // ---- Rebuild payload ----
  char payload[128];
  int written = snprintf(payload, sizeof(payload), "{\"action\":\"%s\",\"timestamp\":%ld,\"nonce\":\"%s\"}", action, timestamp, nonce);

  if (written <= 0 || written >= sizeof(payload)) {
    request->send(500, "text/plain", F("Payload error"));
    return;
  }

  // ---- HMAC ----
  char expected[65];
  hmac_sha256(payload, SECRET, expected);

  // ---- Constant-time compare ----
  if (!constantTimeCompare(expected, sig, 64)) {
    request->send(403, "text/plain", F("Invalid signature"));
    return;
  }

  // ---- Action ----
  if (strcmp(action, "motor_on") == 0) {
    digitalWrite(MOTOR, HIGH);
  } else if (strcmp(action, "motor_off") == 0) {
    digitalWrite(MOTOR, LOW);
  } else {
    request->send(400, "text/plain", F("Invalid action"));
    return;
  }

  request->send(200, "text/plain", F("OK"));
}

void web_dash_init() {
  memset(nonceTable, 0, sizeof(nonceTable));
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", F("<h1>Web Dashboard</h1><br><p>Work in progress......</p>"));
  });
  // server.on("/update", HTTP_POST, handleUpdate, handleUpload);
  server.on("/status", HTTP_GET, handleStatusRequest);

  server.on(
    "/api/7f3a9c_motor_ctrl", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, handleSystemCtrl);
  server.begin();
}
