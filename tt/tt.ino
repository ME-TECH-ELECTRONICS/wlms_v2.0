#include <SPI.h>
#include <LoRa.h>


void setup() {
  Serial.begin(115200);
  LoRa.setPins(10, 9, 2);
  if (!LoRa.begin(433E6)) {  // Change frequency if needed (e.g., 868E6 / 915E6)
    Serial.println("LoRa init failed!");
    while (1)
      ;
  }

  LoRa.setSyncWord(0xF3);
  LoRa.enableCrc();
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(8);
  // Set code word
  Serial.println("LoRa Transmitter Started");
}

struct SensorPacket {
  uint8_t level;
  int8_t temp;
  uint8_t checksum;
};

void sendSensorData(uint8_t level, int8_t temp) {

  SensorPacket pkt;

  pkt.level = level;
  pkt.temp = temp;
  pkt.checksum = pkt.level ^ pkt.temp ^ 0xA5;

  LoRa.beginPacket();
  LoRa.write((uint8_t*)&pkt, sizeof(pkt));
  LoRa.endPacket();

  // Debug
  Serial.print("Sent -> Level: ");
  Serial.print(level);
  Serial.print("% | Temp: ");
  Serial.print(temp);
  Serial.println(" C");
}

void loop() {
  for (uint8_t i = 100; i > 8; i--) {
    sendSensorData(i, 35);
    delay(1000);
  }

  for (uint8_t j = 8; j <= 100; j++) {
    sendSensorData(j, 35);
    delay(1000);
  }
}