#include "lora_rx.h"
#include <LoRa.h>
#include "system.h"

void onReceive(int packetSize) {
  if (packetSize != sizeof(SensorPacket))
    return;

  SensorPacket pkt;
  LoRa.readBytes((uint8_t *)&pkt, sizeof(pkt));
  xQueueSend(loraQueue, &pkt, 0);
}

void initLoara() {
  LoRa.setPins(5, 16, 4);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  LoRa.setSyncWord(0xF3);
  LoRa.enableCrc();
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.onReceive(onReceive);
  LoRa.receive();
  Serial.println("LoRa Ready");
}

void loraTask(void *pv) {
  initLoara();
  SensorPacket pkt;
  while (true) {
    if (xQueueReceive(loraQueue, &pkt, portMAX_DELAY)) {
      if ((pkt.level ^ pkt.temp ^ 0xA5) == pkt.checksum) {
        sys.level = pkt.level;
        sys.lastLevelUpdate = millis();
      }
    }
  }
}
