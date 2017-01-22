#include <SoftwareSerial.h>
#include <Wire.h>
#include "LowPower.h" // https://github.com/rocketscream/Low-Power

#define pinExternalPower 2
#define pinStatusLed 3
#define pinNunchukEnable 17
#define pinBtUartTx 8
#define pinBtUartRx 9

#define hwUartBaudrate 9600
#define atUartBaudrate 9600
#define nunchukI2cAddress 0x52
#define voltageBatteryLowThreshold 3300
#define sleepThreshold 10
#define nunchukPacketSize 6
#define atResponseSize 256

static uint8_t nunchukPacket[ nunchukPacketSize ];
static uint8_t atResponse[ atResponseSize ];
static bool isActive = true;

SoftwareSerial atSerial( pinBtUartRx, pinBtUartTx );

// ------------------------------------------------------------------- Bluetooth

void bluetoothSleep( void )
{
  atSerial.print( "AT+SLEEP" );
  atSerial.flush();
}

void bluetoothWakeup( void )
{
  uint8_t i;

  for( i = 0; i < 100; i++ )
  {
    atSerial.write( i );
  }
  atSerial.flush();
}

void bluetoothSendNunchukPacket( void )
{
  uint8_t i;
  
  for( i = 0; i < nunchukPacketSize; i++ )
  {
    atSerial.write( nunchukPacket[ i ] );
  }
  atSerial.flush();
}

uint8_t bluetoothGetResponse( void )
{
  uint8_t responseLength;
  
  for( responseLength = 0; atSerial.available() > 0 && responseLength < atResponseSize - 1; responseLength++ )
  {
    atResponse[ responseLength ] = Serial.read();
  }
  atResponse[ responseLength ] = 0;
  
  return responseLength;
}

// ------------------------------------------------------------------- Nunchuk

void nunchukPowerOn( void )
{
  digitalWrite( pinNunchukEnable, HIGH );
}

void nunchukInit( void )
{
  uint8_t i = 0;
  
  Wire.beginTransmission( nunchukI2cAddress );
  Wire.write( 0xF0 );
  Wire.write( 0x55 );
  Wire.write( 0xFB );
  Wire.write( 0x00 );
  Wire.endTransmission();

  while( i < nunchukPacketSize )
  {
    nunchukPacket[ i++ ] = 0x00;
  }
}

bool nunchukUpdate( void )
{
  uint8_t i = 0;
  static uint16_t unchangedCnt = 0;
  
  Wire.requestFrom( nunchukI2cAddress, nunchukPacketSize );
  while( Wire.available() )
  {
    uint8_t val = Wire.read();
    if( ( ( i == 0 || i == 1 ) && ( val != nunchukPacket[ i ] ) ) ||
        ( ( i == 5 ) && ( ( val & 0x03 ) != ( nunchukPacket[ i ] & 0x03 ) ) ) )
    {
      unchangedCnt = 0;
    }
    else if( unchangedCnt < sleepThreshold )
    {
      unchangedCnt += 1;
    }
    nunchukPacket[ i++ ] = val;
  }
  
  Wire.beginTransmission( nunchukI2cAddress );
  Wire.write( 0x00 );
  Wire.endTransmission();

  return ( unchangedCnt == sleepThreshold );
}

int8_t nunchukGetJoyX( void )
{
  return ( nunchukPacket[ 0 ] - 127 );
}

int8_t nunchukGetJoyY( void )
{
  return ( nunchukPacket[ 1 ] - 127 );
}

int8_t nunchukGetAccX( void )
{
  return ( nunchukPacket[ 2 ] - 127 );
}

int8_t nunchukGetAccY( void )
{
  return ( nunchukPacket[ 3 ] - 127 );
}

int8_t nunchukGetAccZ( void )
{
  return ( nunchukPacket[ 4 ] - 127 );
}

bool nunchukIsPressedKeyZ( void )
{
  return ( 0 == ( nunchukPacket[ 5 ] & 0x01 ) );
}

bool nunchukIsPressedKeyC( void )
{
  return ( 0 == ( ( nunchukPacket[ 5 ] >> 1 ) & 0x01 ) );
}

void nunchukPowerOff( void )
{
  digitalWrite( pinNunchukEnable, LOW );
}

// ------------------------------------------------------------------- External connection

bool externalIsConnected( void )
{
  return ( digitalRead( pinExternalPower ) != LOW );
}

// ------------------------------------------------------------------- Led

void ledOn( void )
{
  digitalWrite( pinStatusLed, HIGH );
}

void ledOff( void )
{
  digitalWrite( pinStatusLed, LOW );
}

void ledBlink( void )
{
  digitalWrite( pinStatusLed, HIGH );
  delay( 1 );
  digitalWrite( pinStatusLed, LOW );
}

// ------------------------------------------------------------------- Voltage

uint16_t voltageMeasure( void )
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

bool voltageIsCritical( void )
{
  return ( voltageBatteryLowThreshold > voltageMeasure() );
}

// ------------------------------------------------------------------- State machine

void loopActive( void )
{
  bool timedOut = nunchukUpdate();
  if( timedOut )
  {
    gotoStandby();
  }
  else
  {
    ledBlink();
    bluetoothSendNunchukPacket();
    LowPower.powerDown( SLEEP_500MS, ADC_OFF, BOD_OFF );
  }
}

void loopStandby( void )
{
  bool hasUnchangedData = false;
  nunchukPowerOn();
  LowPower.powerDown( SLEEP_60MS, ADC_OFF, BOD_OFF );
  hasUnchangedData = nunchukUpdate();
  if( hasUnchangedData )
  {
    nunchukPowerOff();
  }
  else
  {
    gotoActive();
  }
}

void gotoActive( void )
{
  uint8_t i;

  bluetoothWakeup();
  nunchukPowerOn();
  for( i = 0; i < 4; i++ )
  {
    ledBlink();
    LowPower.powerDown( SLEEP_250MS, ADC_OFF, BOD_OFF );
  }
  isActive = true;
}

void gotoStandby( void )
{
  bluetoothSleep();
  nunchukPowerOff();
  isActive = false;
}

// ------------------------------------------------------------------- Main

void setup( void )
{
  Wire.begin();
  Serial.begin( hwUartBaudrate );
  atSerial.begin( atUartBaudrate );

  pinMode( pinExternalPower, INPUT );
  pinMode( pinStatusLed, OUTPUT );
  pinMode( pinNunchukEnable, OUTPUT );
}

void loop( void )
{
  if( externalIsConnected() )
  {
    ledOn();
  }
  else
  {
    ledOff();
    
    if( isActive )
    {
      loopActive();
    }
    else
    {
      loopStandby();
    }
  }
}
