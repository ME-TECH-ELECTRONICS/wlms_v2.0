#include "web_dash.h"

extern "C" {
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "mbedtls/pk.h"
#include "mbedtls/sha256.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
}

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <SPIFFS.h>
#include "index_html.h"
#include "config.h"

static AsyncWebServer server(80);

// ================= GLOBAL =================
File otaFile;
String metadataStr = "";
String signatureStr = "";

// ================= SHA256 =================
String sha256File(const char* path) {
  File file = SPIFFS.open(path, FILE_READ);
  if (!file) {
    Serial.println("❌ Failed to open file");
    return "";
  }

  Serial.printf("📦 File size: %u bytes\n", file.size());

  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);

  uint8_t buf[1024];

  while (true) {
    int len = file.read(buf, sizeof(buf));
    if (len <= 0) break;

    mbedtls_sha256_update(&ctx, buf, len);
  }

  uint8_t hash[32];
  mbedtls_sha256_finish(&ctx, hash);
  mbedtls_sha256_free(&ctx);
  file.close();

  char out[65];
  for (int i = 0; i < 32; i++) sprintf(out + i * 2, "%02x", hash[i]);
  out[64] = 0;

  return String(out);
}

// ================= SIGNATURE VERIFY =================
bool verifySignature(const String& metadata, const String& sig_b64) {
  uint8_t sig[128];
  size_t sig_len;

  if (mbedtls_base64_decode(sig, sizeof(sig), &sig_len,
      (const unsigned char*)sig_b64.c_str(), sig_b64.length()) != 0)
    return false;

  uint8_t hash[32];
  mbedtls_sha256((const unsigned char*)metadata.c_str(), metadata.length(), hash, 0);

  mbedtls_pk_context pk;
  mbedtls_pk_init(&pk);

  if (mbedtls_pk_parse_public_key(&pk,
      (const unsigned char*)PUBLIC_KEY,
      strlen(PUBLIC_KEY) + 1) != 0) {
    mbedtls_pk_free(&pk);
    return false;
  }

  int ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, 0, sig, sig_len);

  mbedtls_pk_free(&pk);
  return (ret == 0);
}

// ================= OTA UPLOAD =================
void handleUpload(AsyncWebServerRequest *request,
                  String filename,
                  size_t index,
                  uint8_t *data,
                  size_t len,
                  bool final) {

  // if (index == 0) {
  //   Serial.println("Upload Start");

  //   if (SPIFFS.exists("/update.bin"))
  //     SPIFFS.remove("/update.bin");

  //   otaFile = SPIFFS.open("/update.bin", FILE_WRITE);
  // }

  // if (otaFile) otaFile.write(data, len);
  // Serial.printf("Writing %u bytes at index %u\n", len, index);
  // if (final) {
  //   if (otaFile) {
  //     otaFile.flush(); 
  //     otaFile.close();
  //   }
  //   Serial.println("Upload Complete (SPIFFS)");
  // }

  const esp_partition_t* part = esp_ota_get_next_update_partition(NULL);

  if (!Update.begin(part->size)) {
    Serial.println("invalid partition");
    return;
  }
  Serial.printf("available partition: %s\n", part->label);
  if (Update.write(data, len) != len) {
    Serial.println("Something went wrong");
    return;
  }
   delay(1000);
  ESP.restart();
}

// ================= FLASH =================
bool flashFromSPIFFS() {
  File file = SPIFFS.open("/update.bin");
  if (!file) {
    Serial.println("flash func 1");
    return false;
  }

  const esp_partition_t* part = esp_ota_get_next_update_partition(NULL);
  if (!Update.begin(part->size)) {
    file.close();
    Serial.println("flash func 2");
    return false;
  }

  uint8_t buf[1024];
  while (file.available()) {
    size_t len = file.read(buf, sizeof(buf));

    if (Update.write(buf, len) != len) {
      Serial.println("❌ Write failed");
      file.close();
      return false;
    }

    yield();           // ✅ VERY IMPORTANT
  }

  file.close();

  if (!Update.end(true)) {
    Serial.print("issue with the update");
    return false;
  }
  Serial.print("update sucessfull");

  return true;
}

// ================= UPDATE HANDLER =================
void handleUpdate(AsyncWebServerRequest *request) {

  // if (!request->hasParam("metadata", true) ||
  //     !request->hasParam("signature", true)) {
  //   request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing metadata/signature\"}");
  //   return;
  // }

  // metadataStr = request->getParam("metadata", true)->value();
  // signatureStr = request->getParam("signature", true)->value();
  // delay(10); 
  // // 🔐 Verify signature
  // if (!verifySignature(metadataStr, signatureStr)) {
  //   request->send(403, "application/json", "{\"success\":false,\"message\":\"Invalid signature\"}");
  //   SPIFFS.remove("/update.bin");
  //   return;
  // }

  // // 🔴 TEMP: metadata = hash
  // String expectedHash = "";

  // int hashIndex = metadataStr.indexOf("\"hash\":\"");
  // if (hashIndex != -1) {
  //   int start = hashIndex + 8;
  //   int end = metadataStr.indexOf("\"", start);
  //   if (end != -1) {
  //     expectedHash = metadataStr.substring(start, end);
  //   }
  // }

  // String actualHash = sha256File("/update.bin");

  // Serial.println("Expected: " + expectedHash);
  // Serial.println("Actual:   " + actualHash);

  // if (expectedHash != actualHash) {
  //   request->send(403, "application/json", "{\"success\":false,\"message\":\"Hash mismatch\"}");
  //   SPIFFS.remove("/update.bin");
  //   return;
  // }

  // // 🚀 Flash
  // if (!flashFromSPIFFS()) {
  //   request->send(500, "application/json", "{\"success\":false,\"message\":\"Flashing failed\"}");
  //   SPIFFS.remove("/update.bin");
  //   return;
  // }
  // otaReady = true;
  // SPIFFS.remove("/update.bin");

  request->send(200, "application/json", "{\"success\":true,\"message\":\"Update OK, rebooting...\"}");
 
}

void handleStatusRequest(AsyncWebServerRequest *request) {
  char json[128];
  const esp_partition_t* running = esp_ota_get_running_partition();
  Serial.printf("Running partition: %s\n", running->label);
  snprintf(json, sizeof(json), "{\"heap\":%u,\"version\":\"%s\"}", ESP.getFreeHeap(), VERSION);
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
  server.on("/update", HTTP_POST, handleUpdate, handleUpload);
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
