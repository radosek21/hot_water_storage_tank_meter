#include "peripherals.h"


#define TM1637_CLK    ( D6 )
#define TM1637_DIO    ( D5 )


//-------------------------------------------------------------------------
// Display
//-------------------------------------------------------------------------
Display::Display():
tm(TM1637_CLK, TM1637_DIO),
progressCnt(0)
{
  tm.init();
  tm.set(2); // Set brightness level
}

void Display::code(uint8_t BitAddr, int8_t code)
{
  tm.start();          //start signal sent to TM1637 from MCU
  tm.writeByte(ADDR_FIXED);//
  tm.stop();           //
  tm.start();          //
  tm.writeByte(BitAddr|0xc0);//
  tm.writeByte(code + ((tm._PointFlag == POINT_ON) ? 0x80 : 0x00));//
  tm.stop();            //
  tm.start();          //
  tm.writeByte(tm.Cmd_DispCtrl);//
  tm.stop();           //
}

void Display::code(const uint8_t *codes)
{
  code((uint8_t*)codes);
}

void Display::code(uint8_t *codes)
{
  for (int i=0; i < 4; i++) {
     code(i, codes[i]);
  }
}

void Display::number(int num, int pos)
{
  tm.point(false);
  for (int i = 0; i < pos; i++) {
    code(4-pos, DISP_CHAR_Empty);
  }
  if (num >= 1) {
    tm.display(3-pos, num % 10);
  } else {
    code(3-pos, DISP_CHAR_Empty);
  }
  if (num >= 10) {
    tm.display(2-pos, num / 10 % 10);   
  } else {
    code(2-pos, DISP_CHAR_Empty);
  }
  if (num >= 100) {
    tm.display(1-pos, num / 100 % 10);   
  } else {
    code(1-pos, DISP_CHAR_Empty);
  }
  if (num >= 1000) {
    tm.display(0-pos, num / 1000 % 10);
  } else {
    code(0-pos, DISP_CHAR_Empty);
  }
}

void Display::progressIter()
{
  for (int i = 0; i < DISP_COUNT; i++) {
    code(i, progressPattern[progressCnt][i]);
  }
  progressCnt++;
  if (progressCnt >= 10) {
    progressCnt = 0;
  }
}

//-------------------------------------------------------------------------
// Temperature sensors
//-------------------------------------------------------------------------
TempSensors::TempSensors():
oneWire(2),
ds18b20(&oneWire)
{

}

void TempSensors::begin()
{
  memset(bindings, 255, TEMP_SENSORS_CNT);
  ds18b20.begin();
  Serial.println("Locating devices...");
  Serial.print("Found ");
  deviceCount = ds18b20.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");
  Serial.println("Printing addresses...");
  for (int i = 0;  i < deviceCount;  i++) {
    Serial.print("Sensor ");
    Serial.print(i+1);
    Serial.print(" : ");
    ds18b20.getAddress(deviceAddress[i], i);
    printAddress(deviceAddress[i]);
  }
}

bool TempSensors::bind(DeviceAddress addr, uint8_t id)
{
  bool result = false;
  for (int i = 0; i < TEMP_SENSORS_CNT; i++) {
    if (memcmp(addr, deviceAddress[i], 8) == 0) {
      bindings[id] = i;
      Serial.print("Bound ");
      Serial.print(i);
      Serial.print(" to ");
      Serial.println(id);
      result = true;
    }
  }
  return result;
}

float TempSensors::getTemp(uint8_t index)
{
  if(bindings[index] == 255) {
    return DEVICE_DISCONNECTED_C;
  }
  return ds18b20.getTempCByIndex(bindings[index]);
}

void TempSensors::printAddress(uint8_t *addr)
{ 
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (addr[i] < 16) Serial.print("0");
    Serial.print(addr[i], HEX);
  }
  Serial.println("");
}

//-------------------------------------------------------------------------
// Peripherals singletons
//-------------------------------------------------------------------------
Display display;
TempSensors tempSensors;
