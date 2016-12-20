#define remotePowerSensePin 2
#define statusLedPin 3

void setup() {
  pinMode(statusLedPin, OUTPUT);
  pinMode(remotePowerSensePin, INPUT);
}

void loop() {
  digitalWrite(statusLedPin, HIGH);
  if( digitalRead(remotePowerSensePin) == LOW )
    delay(1);
  else
    delay(100);
  digitalWrite(statusLedPin, LOW);
  delay(100);
}
