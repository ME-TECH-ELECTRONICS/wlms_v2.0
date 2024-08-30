#include <SPI.h>
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
    
}