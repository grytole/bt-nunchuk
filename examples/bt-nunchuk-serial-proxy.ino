#include <SoftwareSerial.h>

#define hwUartBaudrate 19200
#define btUartBaudrate 9600
#define btUartTxPin 8
#define btUartRxPin 9

SoftwareSerial btSerial(btUartRxPin, btUartTxPin);

void setup() {
  Serial.begin(hwUartBaudrate);
  btSerial.begin(btUartBaudrate);

  Serial.println("[bluetooth proxy]");
}

void loop() {
  if (btSerial.available()) {
    Serial.write(btSerial.read());
  }
  if (Serial.available()) {
    btSerial.write(Serial.read());
  }
}
