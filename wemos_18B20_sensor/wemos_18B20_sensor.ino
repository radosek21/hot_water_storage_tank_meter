// LOLIN(WEMOS) D1 mini (clone)

#include <ESP8266WiFi.h>
#include <ModbusIP_ESP8266.h>
#include "peripherals.h"

#define HEATER_TANK_MIN_AVG_TEMP    45
#define HEATER_TANK_MAX_AVG_TEMP    77

#define MODBUS_DATA_IREG_BASE_ADDR  100

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

void setup() {
  int timeout = millis() + 60000;
  Serial.begin(115200);
  Serial.println("");

  delay(1000);
  
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
  mb.addIreg(MODBUS_DATA_IREG_BASE_ADDR, 0,  TEMP_SENSORS_CNT); 
  
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
        mb.Ireg(MODBUS_DATA_IREG_BASE_ADDR + i, (int16_t)(temp * 100));
        avgTemp += temp;
        avgCnt++;
      }
    }
    // Do an average temperature 
    avgTemp /= avgCnt;
    // Transform measured temperature value to the percentage of heater tank capacity
    heatCap = ((avgTemp - HEATER_TANK_MIN_AVG_TEMP) * 100 / (HEATER_TANK_MAX_AVG_TEMP - HEATER_TANK_MIN_AVG_TEMP));
    if (heatCap > 0) {
      // Show current capacity
      display.number((int)constrain(heatCap, 0, 100), 1);
    } else {
      // There is no capacity. Display "---"
      display.code(display.emptyPattern);
    }
  }
}

int StrToHex(String str) {
  char buf[3];
  str.toCharArray(buf, 3);
  return (int)strtol(buf, 0, 16);
}
