// LOLIN(WEMOS) D1 mini (clone)

#include <ESP8266WiFi.h>
#include <ModbusIP_ESP8266.h>
#include "peripherals.h"
#include <EEPROM.h>

#define EEPROM_MAGIC_NUMBER                 0xDEADBEEF
#define DEFAULT_HEATER_TANK_MIN_AVG_TEMP    40
#define DEFAULT_HEATER_TANK_MAX_AVG_TEMP    77

#define MB_IREG_SENSORS_CNT_ADDR   1
#define MB_IREG_AVG_TEMP_ADDR      2
#define MB_IREG_CAPACITY_ADDR      3
#define MB_IREG_SENSORS_ADDR       100
#define MB_HREG_MIN_TEMP_ADDR      1
#define MB_HREG_MAX_TEMP_ADDR      2


typedef struct {
  uint32_t magic;
  uint16_t minAvgTemp;
  uint16_t maxAvgTemp;
} EepromData_t;


// Set WiFi credentials
#define WIFI_SSID "MY SSID"
#define WIFI_PASS "MY PASSWORD"

// List of the temperature sensors addresses
String sensorsAddr[TEMP_SENSORS_CNT] = {
  "2801ED0F00000085",
  "283AEC0F0000005D",
  "285BF60F00000068",
  "2827FF0F0000007E",
  "28D7FD0F0000008F",
  "2870FB0F000000CA",
  "2804F10F00000033",
  "2819FE0F0000004D",
  "28A2F00F00000010",
  "28CDF70F000000A6",
};

// Put temperature corrections here, if needed.
float sensorsCorrections[TEMP_SENSORS_CNT] = {
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
};

ModbusIP mb;
long ts;
EepromData_t eepromData;
uint16_t iRegAvgTemp;
uint16_t iRegSensorsCnt;

void setup() {
  int timeout = millis() + 60000;
  iRegAvgTemp = 0;
  iRegSensorsCnt = 0;
  Serial.begin(115200);
  Serial.println("");
  
  
  delay(1000);
  readParams();
  tempSensors.begin();

  // Begin WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  // Connecting to WiFi...
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  // Loop continuously while WiFi is not connected
  while ((WiFi.status() != WL_CONNECTED) && (millis() < timeout)) {
    delay(100);
    Serial.print(".");
    display.progressIter();
  }

  Serial.println();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WIFI connection timed out!");
  }

  // Start Modbus TCP server
  mb.server();    
  // Register modbus input registers
  mb.addIreg(MB_IREG_SENSORS_CNT_ADDR, 0);
  mb.addIreg(MB_IREG_AVG_TEMP_ADDR, 0);
  mb.addIreg(MB_IREG_CAPACITY_ADDR, 0);
  mb.addIreg(MB_IREG_SENSORS_ADDR, 0, TEMP_SENSORS_CNT); 
  mb.addHreg(MB_HREG_MIN_TEMP_ADDR, eepromData.minAvgTemp);
  mb.addHreg(MB_HREG_MAX_TEMP_ADDR, eepromData.maxAvgTemp);
  mb.onSetHreg(MB_HREG_MIN_TEMP_ADDR, cbWriteMinTemp, 1);
  mb.onSetHreg(MB_HREG_MAX_TEMP_ADDR, cbWriteMaxTemp, 1);
  
  // Bind all DS18B20 temperature sensors
  for (int index = 0; index < TEMP_SENSORS_CNT; index++) {
    DeviceAddress addr;
    for (int i = 0; i < 8; i++ ) {
      addr[i] = StrToHex(sensorsAddr[index].substring(i*2, i*2+2)); 
    }
    if (!tempSensors.bind(addr, index)) {
      Serial.print("Sensor address ");
      Serial.print(sensorsAddr[index]);
      Serial.println(" not bound!!!");
    }
  }
  ts = millis();
}

void loop() {
  float temp;
  float avgTemp = 0;
  int avgCnt = 0;
  float heatCap;
  
  // Modbus TCP handler
  mb.task();
  
  //Read every two seconds
  if (millis() >= ts) {
    // Schedule next temperature sensors reading
    ts = millis() + 2000;
    
    // Send command to all the sensors for temperature conversion
    tempSensors.requestTemperatures();
    // 
    for (int i = 0;  i < TEMP_SENSORS_CNT;  i++) {
      temp = tempSensors.getTemp(i) + sensorsCorrections[i];
      // If temperature value is valid
      if ((temp > 0) && (temp < 120)) {
        Serial.print("Sensor ");
        Serial.print(i+1);
        Serial.print(" : ");
        Serial.print(temp);
        Serial.println("C");
        mb.Ireg(MB_IREG_SENSORS_ADDR + i, (int16_t)(temp * 100));
        avgTemp += temp;
        avgCnt++;
      }
    }
    mb.Ireg(MB_IREG_SENSORS_CNT_ADDR, avgCnt);  
    if (avgCnt > (TEMP_SENSORS_CNT / 2)) {
      // Do an average temperature when at least half of sensors are present
      avgTemp /= avgCnt;
      // Transform measured temperature value to the percentage of heater tank capacity
      heatCap = ((avgTemp - eepromData.minAvgTemp) * 100 / (eepromData.maxAvgTemp - eepromData.minAvgTemp));
      mb.Ireg(MB_IREG_AVG_TEMP_ADDR, (int16_t)(avgTemp * 100));
      mb.Ireg(MB_IREG_CAPACITY_ADDR, (int16_t)(heatCap * 100));
      // Show current capacity
      display.number((int)constrain(heatCap, -150, 999), 1);
    } else {
      // Half of sensors are missed. Display "---"
      display.code(display.emptyPattern);
      mb.Ireg(MB_IREG_AVG_TEMP_ADDR, 0);
      mb.Ireg(MB_IREG_CAPACITY_ADDR, 0);
    }
  }
}

int StrToHex(String str) {
  char buf[3];
  str.toCharArray(buf, 3);
  return (int)strtol(buf, 0, 16);
}

void readParams()
{
  //Init EEPROM
  EEPROM.begin(sizeof(EepromData_t));
  Serial.println("EEPROM: reading");
  EEPROM.get(0, eepromData);
  if(eepromData.magic != EEPROM_MAGIC_NUMBER) {
    Serial.println("EEPROM: using defaults.");
    eepromData.magic = EEPROM_MAGIC_NUMBER;
    eepromData.minAvgTemp = DEFAULT_HEATER_TANK_MIN_AVG_TEMP;
    eepromData.maxAvgTemp = DEFAULT_HEATER_TANK_MAX_AVG_TEMP;
  }
  EEPROM.end();
}

void writeParams()
{
  //Init EEPROM
  Serial.println("EEPROM: writing");
  EEPROM.begin(sizeof(EepromData_t));
  EEPROM.put(0, eepromData);
  EEPROM.commit();
  EEPROM.end();
}

uint16_t cbWriteMinTemp(TRegister* reg, uint16_t val) {
  Serial.print("Write min temp: ");
  Serial.println(val);
  eepromData.minAvgTemp = val;
  writeParams();
  return val;
}

uint16_t cbWriteMaxTemp(TRegister* reg, uint16_t val) {
  Serial.print("Write max temp: ");
  Serial.println(val);
  Serial.println(val);
  eepromData.maxAvgTemp = val;
  writeParams();
  return val;
}
