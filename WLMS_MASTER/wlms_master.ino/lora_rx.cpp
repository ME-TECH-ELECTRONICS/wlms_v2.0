#include "lora_rx.h"
#include <LoRa.h>
#include "system.h"

uint32_t lastPacketTime = 0;

void initLoara() {
  LoRa.setPins(5, 16, 4);
  LoRa.setSyncWord(0xF3);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");

    while (true) {
      vTaskDelay(pdMS_TO_TICKS(1000));  // ✅ prevent WDT reset
    }
  }
  LoRa.enableCrc();
}

void handleLoRa() {

  int packetSize = 0;

  // 🔒 SPI LOCK (parse)
  if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(10))) {
    packetSize = LoRa.parsePacket();
    xSemaphoreGive(spiMutex);
  } else {
    return;
  }

  if (packetSize == sizeof(SensorPacket)) {

    SensorPacket pkt;

    // 🔒 SPI LOCK (read)
    if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(20))) {

      uint8_t *ptr = (uint8_t *)&pkt;

      for (int i = 0; i < sizeof(SensorPacket); i++) {
        if (!LoRa.available()) {
          xSemaphoreGive(spiMutex);
          return;  // incomplete packet → discard
        }
        ptr[i] = LoRa.read();
      }

      xSemaphoreGive(spiMutex);

    } else {
      return;
    }

    // Validate checksum
    if ((pkt.level ^ pkt.temp ^ 0xA5) != pkt.checksum) {
      Serial.println("Checksum error!");
      return;
    }

    uint32_t now = millis();

    // 🔒 SYS MUTEX (VERY SHORT)
    if (xSemaphoreTake(sysMutex, pdMS_TO_TICKS(5))) {
      sys.level = pkt.level;
      sys.lastLevelUpdate = now;
      xSemaphoreGive(sysMutex);
      // Serial.printf("RX Level: %d\n", pkt.level);
    } else {
      // Optional: skip update if busy (rare)
      return;
    }

    lastPacketTime = now;

  }

  else if (packetSize > 0) {
    // 🔒 Flush unknown packet
    if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(10))) {
      while (LoRa.available()) LoRa.read();
      xSemaphoreGive(spiMutex);
    }
  }
}

void loraTask(void *pv) {
  // 🔒 Protect SPI during init
  if (xSemaphoreTake(spiMutex, portMAX_DELAY)) {
    initLoara();
    xSemaphoreGive(spiMutex);
  }

  while (true) {
    handleLoRa();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}