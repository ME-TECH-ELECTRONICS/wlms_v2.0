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

// uint32_t lastPacketTime = 0;

// void initLoara() {
//   LoRa.setPins(5, 16, 4);
//   LoRa.setSyncWord(0xF3);

//   if (!LoRa.begin(433E6)) {
//     Serial.println(F("LoRa init failed!"));

//     while (true) {
//       vTaskDelay(pdMS_TO_TICKS(1000));  // ✅ prevent WDT reset
//     }
//   }
//   LoRa.setSpreadingFactor(7);
//   LoRa.setSignalBandwidth(125E3);
//   LoRa.setCodingRate4(5);
//   LoRa.setPreambleLength(8);
//   LoRa.enableCrc();
// }

// void handleLoRa() {

//   //   if (!xSemaphoreTake(spiMutex, pdMS_TO_TICKS(20))) {
//   //     Serial.println("SPI MUTEX TIMEOUT");
//   //     return;
//   //   }

//   int packetSize = LoRa.parsePacket();

//   if (packetSize == sizeof(SensorPacket)) {
//     Serial.printf("Packet detected size=%d\n", packetSize);

//     SensorPacket pkt;

//     LoRa.readBytes((uint8_t *)&pkt, sizeof(pkt));

//     // xSemaphoreGive(spiMutex);
//     Serial.printf(
//       "RX=%d RSSI=%d SNR=%.2f\n",
//       pkt.level,
//       LoRa.packetRssi(),
//       LoRa.packetSnr());

//     if ((pkt.level ^ pkt.temp ^ 0xA5) != pkt.checksum)
//       return;

//     // Serial.printf("LORA update level=%d\n", pkt.level);

//     // if (xSemaphoreTake(sysMutex, pdMS_TO_TICKS(20))) {
//     sys.level = pkt.level;
//     sys.lastLevelUpdate = millis();
//     Serial.printf("LORA level=%d lastUpdate=%lu\n", pkt.level, millis());
//     //   xSemaphoreGive(sysMutex);
//     // } else {

//     //   Serial.println("sysMutex timeout");
//     // }
//   } else {
//     while (LoRa.available())
//       LoRa.read();

//     // xSemaphoreGive(spiMutex);
//   }
// }

// // void handleLoRa() {
// //   int packetSize = 0;
// //   // 🔒 SPI LOCK (parse)
// //   if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(10))) {
// //     packetSize = LoRa.parsePacket();
// //     xSemaphoreGive(spiMutex);
// //   } else {
// //     return;
// //   }

// //   if (packetSize >= sizeof(SensorPacket)) {
// //     SensorPacket pkt;
// //     // 🔒 SPI LOCK (read)
// //     if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(20))) {
// //       uint8_t *ptr = (uint8_t *)&pkt;
// //       for (int i = 0; i < sizeof(SensorPacket); i++) {
// //         if (!LoRa.available()) {
// //           xSemaphoreGive(spiMutex);
// //           return;  // incomplete packet → discard
// //         }
// //         ptr[i] = LoRa.read();
// //       }
// //       xSemaphoreGive(spiMutex);
// //     } else {
// //       return;
// //     }
// //     // Validate checksum
// //     if ((pkt.level ^ pkt.temp ^ 0xA5) != pkt.checksum) {
// //       Serial.println(F("Checksum error!"));
// //       return;
// //     }
// //     uint32_t now = millis();
// //     // 🔒 SYS MUTEX (VERY SHORT)
// //     if (xSemaphoreTake(sysMutex, pdMS_TO_TICKS(5))) {
// //       sys.level = pkt.level;
// //       sys.lastLevelUpdate = now;
// //       xSemaphoreGive(sysMutex);
// //       Serial.printf("RX=%d RSSI=%d\n", pkt.level, LoRa.packetRssi());
// //     } else {
// //       return;
// //     }
// //     lastPacketTime = now;
// //   } else if (packetSize > 0) {
// //     // 🔒 Flush unknown packet
// //     if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(10))) {
// //       while (LoRa.available()) LoRa.read();
// //       xSemaphoreGive(spiMutex);
// //     }
// //   }
// // }

// void loraTask(void *pv) {
//   // 🔒 Protect SPI during init
//   if (xSemaphoreTake(spiMutex, portMAX_DELAY)) {
//     initLoara();
//     xSemaphoreGive(spiMutex);
//   }

//   while (true) {
//     handleLoRa();
//     vTaskDelay(pdMS_TO_TICKS(2));
//   }
// }