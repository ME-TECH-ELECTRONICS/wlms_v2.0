#include <SPI.h>
#include <LoRa.h>

//define the pins used by the transceiver module
#define ss 10
#define rst 9
#define dio0 2

int counter = 0;
struct RxData {
    int distance;
};
 RxData data;
int distance;
void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Sender");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  
  //replace the LoRa.begin(---E-) argument with your location's frequency 
  //433E6 for Asia
  //868E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(433E6)) {
    Serial.println(".");
    delay(500);
  }
   // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  if (Serial.available() > 0) {
    // RxData data;
    
    // Read the user input as an integer
    distance = Serial.parseInt();
    
    // Begin packet
    

    // Serial.print("Transmitted distance: ");
    // Serial.println(data.distance);

    // Serial.println("Enter the distance value:");
  }
data.distance = distance;
  LoRa.beginPacket();
  LoRa.write(distance);
    // Transmit data as bytes
    // LoRa.write((uint8_t*)&data, sizeof(data));

    // End packet
    LoRa.endPacket();

    delay(1000);
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