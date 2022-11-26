#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include <Arduino.h>
#include <TM1637.h> // For this purpose install the "TM1637 - Grove 4-Digital Display" library
#include <OneWire.h>
#include <DallasTemperature.h>

//-------------------------------------------------------------------------
// Display
//-------------------------------------------------------------------------
#define DISP_COUNT  ( 3 )
#define DISP_CHAR_Empty ( 0x00 )
class Display
{
  public:
    Display();
    void code(uint8_t BitAddr, int8_t code);
    void code(const uint8_t *codes);
    void code(uint8_t *codes);
    void number(int num, int pos = 0);
    void progressIter();
    const uint8_t progressPattern[10][DISP_COUNT] = {{0x00, 0x00, 0x02}, {0x00, 0x00, 0x04}, {0x00, 0x00, 0x08}, {0x00, 0x08, 0x00}, {0x08, 0x00, 0x00}, {0x10, 0x00, 0x00}, {0x20, 0x00, 0x00}, {0x01, 0x00, 0x00}, {0x00, 0x01, 0x00}, {0x00, 0x00, 0x01}};
    const uint8_t emptyPattern[DISP_COUNT] = {0x40, 0x40, 0x40};
  private:
    TM1637 tm;
    int progressCnt;
};

#define TEMP_SENSORS_CNT 10
class TempSensors
{
  public:
    TempSensors();
    void begin();
    bool bind(DeviceAddress addr, uint8_t id);
    float getTemp(uint8_t index);
    uint8_t getDeviceCount() const { return deviceCount; };
    void requestTemperatures() { ds18b20.requestTemperatures(); }
  private:
    void printAddress(uint8_t *addr);
    OneWire oneWire;
    DallasTemperature ds18b20;
    DeviceAddress deviceAddress[TEMP_SENSORS_CNT];
    uint8_t bindings[TEMP_SENSORS_CNT];
    uint8_t deviceCount;
};

//-------------------------------------------------------------------------
// Peripherals singletons global externs
//-------------------------------------------------------------------------
extern Display display;
extern TempSensors tempSensors;

#endif // PERIPHERALS_H
