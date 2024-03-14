void setup() {
  pinMode(3, INPUT);
  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
}

void loop() {
  if (digitalRead(3) == HIGH) {
    digitalWrite(8, HIGH);
  }
  else {
    digitalWrite(8, LOW);
  }
}
