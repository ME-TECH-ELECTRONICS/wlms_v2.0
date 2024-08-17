
const int trigPin = 0;
const int echoPin = 1;

void setup() {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
}

void loop() {

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