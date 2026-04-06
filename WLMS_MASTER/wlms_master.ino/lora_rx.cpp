#include "lora_rx.h"
#include <LoRa.h>
#include "system.h"

uint32_t lastPacketTime = 0;

void initLoara() {
  LoRa.setPins(5, 16, 4);
  LoRa.setSpreadingFactor(7);      // fast
  LoRa.setSignalBandwidth(250E3);  // stable (or 250E3 if short range)
  LoRa.setCodingRate4(5);          // fast
  LoRa.setTxPower(17);             // strong
  LoRa.setSyncWord(0xF3);  // isolate network
  LoRa.enableCrc();
  if (!LoRa.begin(433E6)) {
    while (1)
      ;
  }
}

void handleLoRa() {

  int packetSize = 0;

  // 🔒 Lock ONLY for parsePacket()
  if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(10))) {
    packetSize = LoRa.parsePacket();
    xSemaphoreGive(spiMutex);
  } else {
    return; // SPI busy → skip this cycle
  }

  if (packetSize == sizeof(SensorPacket)) {

    SensorPacket pkt;

    // 🔒 Lock ONLY for reading bytes
    if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(10))) {

      uint8_t *ptr = (uint8_t*)&pkt;

      for (int i = 0; i < sizeof(SensorPacket); i++) {
        if (LoRa.available()) {
          ptr[i] = LoRa.read();
        }
      }

      xSemaphoreGive(spiMutex);

    } else {
      return; // SPI busy → skip safely
    }

    // 🚀 ---- PROCESS OUTSIDE SPI LOCK ----

    // Validate checksum
    if ((pkt.level ^ pkt.temp) != pkt.checksum) {
      Serial.println("Checksum error!");
      return;
    }

    // Update state (very fast, safe)
    sys.level = pkt.level;
    sys.lastLevelUpdate = millis();
    lastPacketTime = millis();

    // Debug (no SPI here, safe)
    Serial.printf("Level: %d%% | Temp: %d C\n", pkt.level, pkt.temp);
  }

  else if (packetSize > 0) {
    // 🔒 Flush unknown packet quickly
    if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(10))) {
      while (LoRa.available()) LoRa.read();
      xSemaphoreGive(spiMutex);
    }
  }
}

void loraTask(void *pv) {
  while (true) {
    handleLoRa();

    // Small delay → allows SD task to run
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}