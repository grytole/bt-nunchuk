#define uartBaudrate 19200
#define statusLedPin 3

long readVcc()
{
  #define VCC_CONST 1125300L // Calculate Vcc const (in mV); 1125300 = 1.1 * 1023 * 1000
  
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  
  delay(2); // Wait for Vref to settle
  
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  result = VCC_CONST / result;
  return result; // Vcc in millivolts
}

void blink()
{
  digitalWrite(statusLedPin, HIGH);
  delay(1);
  digitalWrite(statusLedPin, LOW);
}

void setup() {
  pinMode(statusLedPin, OUTPUT);
  
  Serial.begin(uartBaudrate);
}

void loop() {
  long vcc = readVcc();
  
  Serial.print("VCC: ");
  Serial.print(vcc, DEC);
  Serial.print(" mV\n");

  if( vcc < 3200)
  {
    blink();
  }
  
  delay(1000);
}
