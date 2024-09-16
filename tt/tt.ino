#include <SPI.h>
#include <LoRa.h>

//define the pins used by the transceiver module
#define ss 5
#define rst 15
#define dio0 2
struct RxData {
    int distance;
};
void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Receiver");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);

  while (!LoRa.begin(433E6)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    RxData receivedData;

    // Read the incoming bytes into the struct
    LoRa.readBytes((uint8_t*)&receivedData, sizeof(receivedData));

    // Print the received distance value
    Serial.print("Received distance: ");
    Serial.println(receivedData.distance);
  }
}