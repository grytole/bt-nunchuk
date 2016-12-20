#include <Wire.h>

#define uartBaudrate 19200
#define nunchukPowerPin 17

static uint8_t nunchuck_buf[6];

void printData()
{
  int val = 0;
  int joyx_coeff = -127;
  int joyy_coeff = -127;
  int accx_coeff = -127;
//  int accx_coeff = -511;
  int accy_coeff = -127;
//  int accy_coeff = -511;
  int accz_coeff = -127;
//  int accz_coeff = -511;

  // print data
  val = (int)nunchuck_buf[0] + joyx_coeff;
  Serial.print("[Joy X] ");
  Serial.print(val, DEC);
  Serial.print("\t");

  val = (int)nunchuck_buf[1] + joyy_coeff;
  Serial.print("[Joy Y] ");
  Serial.print(val, DEC);
  Serial.print("\t");

  val = (int)nunchuck_buf[2] + accx_coeff;
//  val = nunchuck_buf[2] << 2;
//  val += (nunchuck_buf[5] >> 2) & 0x03;
//  val += accx_coeff;
  Serial.print("[Acc X] ");
  Serial.print(val, DEC);
  Serial.print("\t");

  val = (int)nunchuck_buf[3] + accy_coeff;
//  val = nunchuck_buf[3] << 2;
//  val += (nunchuck_buf[5] >> 4) & 0x03;
//  val += accy_coeff;
  Serial.print("[Acc Y] ");
  Serial.print(val, DEC);
  Serial.print("\t");

  val = (int)nunchuck_buf[4] + accz_coeff;
//  val = nunchuck_buf[4] << 2;
//  val += (nunchuck_buf[5] >> 6) & 0x03;
//  val += accz_coeff;
  Serial.print("[Acc Z] ");
  Serial.print(val, DEC);
  Serial.print("\t");

  Serial.print("[Btn Z] ");
  Serial.print((uint8_t)(nunchuck_buf[5] & 0x01), DEC);
  Serial.print("\t");
  Serial.print("[Btn C] ");
  Serial.print((uint8_t)(nunchuck_buf[5] & 0x02) >> 1, DEC);
  Serial.print("\n");
}

/*
void printPlot()
{
  int val = 0;
  int joyx_coeff = -127;
  int joyy_coeff = -127;
  int accx_coeff = -127;
//  int accx_coeff = -511;
  int accy_coeff = -127;
//  int accy_coeff = -511;
  int accz_coeff = -127;
//  int accz_coeff = -511;

  // print data
  val = (int)nunchuck_buf[0] + joyx_coeff;
  Serial.print(val, DEC);
  Serial.print(" ");

  val = (int)nunchuck_buf[1] + joyy_coeff;
  Serial.print(val, DEC);
  Serial.print(" ");

  val = (int)nunchuck_buf[2] + accx_coeff;
//  val = nunchuck_buf[2] << 2;
//  val += (nunchuck_buf[5] >> 2) & 0x03;
//  val += accx_coeff;
  Serial.print(val, DEC);
  Serial.print(" ");

  val = (int)nunchuck_buf[3] + accy_coeff;
//  val = nunchuck_buf[3] << 2;
//  val += (nunchuck_buf[5] >> 4) & 0x03;
//  val += accy_coeff;
  Serial.print(val, DEC);
  Serial.print(" ");

  val = (int)nunchuck_buf[4] + accz_coeff;
//  val = nunchuck_buf[4] << 2;
//  val += (nunchuck_buf[5] >> 6) & 0x03;
//  val += accz_coeff;
  Serial.print(val, DEC);
  Serial.print(" ");

  Serial.print((uint8_t)(nunchuck_buf[5] & 0x01), DEC);
  Serial.print(" ");
  Serial.print((uint8_t)(nunchuck_buf[5] & 0x02) >> 1, DEC);
  Serial.print("\n");
}
*/

void setup()
{
  nunchuck_buf[0] = 0;
  nunchuck_buf[1] = 0;
  nunchuck_buf[2] = 0;
  nunchuck_buf[3] = 0;
  nunchuck_buf[4] = 0;
  nunchuck_buf[5] = 0;

  // enable nunchuk power
  pinMode(nunchukPowerPin, OUTPUT);
  digitalWrite(nunchukPowerPin, HIGH);

  delay(100); // wait some time till nunchuk inits
  
  Serial.begin(uartBaudrate);  // start serial for output
  Wire.begin();        // join i2c bus (address optional for master)

  // init nunchuck
  Wire.beginTransmission(0x52);
  Wire.write(0xF0);
  Wire.write(0x55);
  Wire.write(0xFB);
  Wire.write(0x00);
  Wire.endTransmission();

  Serial.print("[ Wii Nunchuck ready ]\n");
}

void loop()
{
  int cnt = 0;

  // request for new data
  Wire.beginTransmission(0x52);
  Wire.write((uint8_t)0x00);
  Wire.endTransmission();

  // read current data
  Wire.requestFrom (0x52, 6);
  while (Wire.available ()) {
    nunchuck_buf[cnt] = Wire.read();
    cnt++;
  }

  printData();
  //printPlot();

  delay(100);
}
