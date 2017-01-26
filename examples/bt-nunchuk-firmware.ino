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
#define sleepThreshold 1333 // 60s @ 45ms
//#define sleepThreshold 66 // 3s @ 45ms
#define nunchukPacketSize 6
#define atResponseSize 256

static uint8_t nunchukPacket[ nunchukPacketSize ];
static char atResponse[ atResponseSize ];
static bool isActive = false;

SoftwareSerial atSerial( pinBtUartRx, pinBtUartTx );

// ------------------------------------------------------------------- Bluetooth

bool bluetoothSleep( void )
{
  atSerial.print( "AT+SLEEP" );
  atSerial.flush();
  delay(2);
  if( bluetoothCheckResponse( "OK+SLEEP" ) )
  {
      //Serial.println( "BtSleep: OK" ); Serial.flush();
      return true;
  }
  return false;
}

void bluetoothWakeup( void )
{
  uint8_t i;
  for( i = 0; i < 255; i++ )
  {
    atSerial.write( 'x' );
  }
  atSerial.flush();
  delay(2);
  if( bluetoothCheckResponse( "OK+WAKE" ) )
  {
    //Serial.println( "BtWakeup: OK" ); Serial.flush();
    return true;
  }
  return false;
}

bool bluetoothDummy( void )
{
  atSerial.print( "AT" );
  atSerial.flush();
  delay(2);
  if( bluetoothCheckResponse( "OK" ) )
  {
    //Serial.println( "BtDummy: OK" ); Serial.flush();
    return true;
  }
  //Serial.println( "BtDummy: ERR" ); Serial.flush();
  return false;
}

void bluetoothSendNunchukPacket( void )
{
  uint8_t i;

  //Serial.println( "BtSend" ); Serial.flush();
  
  for( i = 0; i < nunchukPacketSize; i++ )
  {
    atSerial.write( nunchukPacket[ i ] );
  }
  atSerial.flush();
}

uint8_t bluetoothGetResponse( void )
{
  uint8_t responseLength;

  //Serial.println( "BtResp" ); Serial.flush();
  
  for( responseLength = 0; atSerial.available() > 0 && responseLength < atResponseSize - 1; responseLength++ )
  {
    atResponse[ responseLength ] = atSerial.read();
  }
  atResponse[ responseLength ] = 0;
  
  return responseLength;
}

bool bluetoothCheckResponse( const char *response )
{
  if( strlen( response ) == bluetoothGetResponse() )
  {
    if( 0 == strcmp( atResponse, response ) )
    {
      return true;
    }
  }
  return false;
}

// ------------------------------------------------------------------- Nunchuk

void nunchukPowerOn( void )
{
  //Serial.println( "JoyOn" ); Serial.flush();
  
  digitalWrite( pinNunchukEnable, HIGH );
}

void nunchukInit( void )
{
  uint8_t i = 0;
  
  //Serial.println( "JoyInit" ); Serial.flush();
  
  Wire.beginTransmission( nunchukI2cAddress );
  Wire.write( 0xF0 );
  Wire.write( 0x55 );
  Wire.write( 0xFB );
  Wire.endTransmission();

  //LowPower.powerDown( SLEEP_15MS, ADC_OFF, BOD_OFF );
  delay(2);

  Wire.beginTransmission( nunchukI2cAddress );
  Wire.write( 0x00 );
  Wire.endTransmission();
}

void nunchukClearBuffer( void )
{
  uint8_t i = 0;
  
  while( i < nunchukPacketSize )
  {
    nunchukPacket[ i++ ] = 0x00;
  }
}

bool nunchukUpdate( void )
{
  uint8_t i = 0;
  bool changed = false;
  static uint16_t unchangedCnt = 0;
  
  //Serial.println( "JoyUpdate" );
  
  Wire.requestFrom( nunchukI2cAddress, nunchukPacketSize );
  while( Wire.available() )
  {
    uint8_t val = Wire.read();

    if( ( ( i == 0 || i == 1 ) && ( val != nunchukPacket[ i ] ) ) ||
        ( ( i == 5 ) && ( ( val & 0x03 ) != ( nunchukPacket[ i ] & 0x03 ) ) ) )
    {
      changed = true;
    }
    
    //Serial.print( changed?"(!) ":"(=) " ); Serial.print( "   [ " ); Serial.print( i, DEC ); Serial.print( " ] = " ); Serial.println( val & ((i == 5)?0x03:0xff), HEX ); Serial.flush();
    
    nunchukPacket[ i++ ] = val;
  }

  if( false == changed )
  {
    if( unchangedCnt < sleepThreshold )
    {
      unchangedCnt += 1;
    }
  }
  else
  {
    unchangedCnt = 0;
  }

  //Serial.print( "JoyUpdate / unchangedCnt: " ); Serial.println( unchangedCnt, DEC ); Serial.flush();

  //LowPower.powerDown( SLEEP_15MS, ADC_OFF, BOD_OFF );
  delay(2);

  Wire.beginTransmission( nunchukI2cAddress );
  Wire.write( 0x00 );
  Wire.endTransmission();

  if( isActive )
  {
    return ( unchangedCnt == sleepThreshold );
  }
  else
  {
    return ( 0 != unchangedCnt );
  }
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
  //Serial.println( "JoyOff" ); Serial.flush();
  
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

  //Serial.print( "************** LoopActive" ); Serial.println( timedOut?": TIMEDOUT":""); Serial.flush();
  
  if( timedOut )
  {
    gotoStandby();
  }
  else
  {
    //ledBlink();
    bluetoothSendNunchukPacket();
    LowPower.powerDown( SLEEP_30MS, ADC_OFF, BOD_OFF );
  }
}

void loopStandby( void )
{
  bool timedOut = false;

  nunchukPowerOn();
  LowPower.powerDown( SLEEP_60MS, ADC_OFF, BOD_OFF );
  nunchukInit();
  //LowPower.powerDown( SLEEP_120MS, ADC_OFF, BOD_OFF );

  //dummy updates
  nunchukUpdate();
  nunchukUpdate();
  
  timedOut = nunchukUpdate();
  nunchukPowerOff();

  //Serial.print( "************** LoopStandby" ); Serial.println( timedOut?" :TIMEDOUT":""); Serial.flush();
  
  if( timedOut )
  {
    LowPower.powerDown( SLEEP_500MS, ADC_OFF, BOD_OFF );
  }
  else
  {
    gotoActive();
  }
}

void gotoActive( void )
{
  uint8_t i;
  
  //Serial.println( "@@@@@@@@@@@@@@@@ gotoActive" ); Serial.flush();

  //for( i = 0; i < 16; i++ )
  //{
    ledBlink();
  //  LowPower.powerDown( SLEEP_60MS, ADC_OFF, BOD_OFF );
  //}

  nunchukPowerOn();
  
  for( i = 10; i > 0; i-- )
  {
    bluetoothWakeup();
    LowPower.powerDown( SLEEP_250MS, ADC_OFF, BOD_OFF );
    if( ( 0 == i ) && ( false == bluetoothDummy() ) )
    {
      return;
    }
    
    if( true == bluetoothDummy() )
    {
      break;
    }
  }
  
  nunchukInit();
  
  //dummy updates
  nunchukUpdate();
  nunchukUpdate();
  
  isActive = true;
}

void gotoStandby( void )
{
  uint8_t i;

  //Serial.println( "@@@@@@@@@@@@@@@@ gotoStandby" ); Serial.flush();
  
  bluetoothSleep();
  nunchukPowerOff();
  //for( i = 0; i < 16; i++ )
  //{
    ledBlink();
  //  LowPower.powerDown( SLEEP_60MS, ADC_OFF, BOD_OFF );
  //}
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

  nunchukClearBuffer();

  LowPower.powerDown( SLEEP_1S, ADC_OFF, BOD_OFF );
  
  gotoActive();
}

void loop( void )
{
//  if( externalIsConnected() )
//  {
//    ledOn();
//  }
//  else
//  {
//    ledOff();
    
    if( isActive )
    {
      loopActive();
    }
    else
    {
      loopStandby();
    }
//  }
}
