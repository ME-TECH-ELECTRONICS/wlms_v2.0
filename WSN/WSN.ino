#include <SPI.h>
#include <LoRa.h>

// define the pins used by the transceiver module
#define ss 10
#define rst 9
#define dio0 2

// Structure to hold the distance
struct RxData {
  int distance;
};
RxData data;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("LoRa Sender");
  LoRa.setPins(ss, rst, dio0);
  while (!LoRa.begin(433E6)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  // Check if new data is available on Serial
  if (Serial.available() > 0) {
    data.distance = Serial.parseInt();
    Serial.print("New distance entered: ");
    Serial.println(data.distance);
    while (Serial.available()) {
      Serial.read();
    }
  }
  LoRa.beginPacket();
  LoRa.write((uint8_t*)&data, sizeof(data));
  LoRa.endPacket();
  delay(1000);  // Send every second
}


/*#include <SPI.h>
#include <RF24.h>

const int trigPin = 0;
const int echoPin = 1;
const byte address[6] = "03152";
unsigned long prevTime;
int dist = 0;

struct DataPacket {
  int distance;
  //float temp;
};

DataPacket data;
RF24 radio(9, 10); // CE, CSN

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
}

void loop() {
  for (int i = 0; i < 5; i++) {
      dist += getDistance();
  }
  data.distance = dist/5;
  if((millis() - prevTime) >= 15000) {
      radio.write(&data, sizeof(data));
  }
}

int getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  return (duration * 0.034 / 2) + 2;
}

float getTemperature() {
    
}*/