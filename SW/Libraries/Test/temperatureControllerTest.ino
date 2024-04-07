/* TEMPERATURES SECTION */
#include <temperatureController.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

#define MAX_REACHABLE_TEMPERATURE_SATURATION 40.0
#define TEMPERATURE_PRECISION 9 // DS18B20 digital termometer provides 9-bit to 12-bit Celsius temperature measurements
#define MAX_SENSORS 10
DeviceAddress Thermometer[MAX_SENSORS];
byte numberOfDevices;
byte Limit;
unsigned long conversionTime_DS18B20_sensors; // in millis
unsigned long lastTempRequest;

#if !defined(DEVICE_DISCONNECTED)
#define DEVICE_DISCONNECTED -127
#endif

// temperature control

temperatureController temperatureController;
/* END TEMPERATURES SECTION */

void setup() {
  Serial.begin(9600);

  /* TEMPERATURES SECTION */
  temperatureController.setTemperatureHysteresis(25.0, 27.0);

  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  lastTempRequest = millis(); 
  conversionTime_DS18B20_sensors = 750 / (1 << (12 - TEMPERATURE_PRECISION));  // res in {9,10,11,12}


  if (numberOfDevices > MAX_SENSORS){
    Limit = MAX_SENSORS;
  }
  else{
    Limit = numberOfDevices;
  }
  
  for(byte index = 0; index < Limit; index++){
    if(sensors.getAddress(Thermometer[index], index)){
      sensors.setResolution(Thermometer[index], TEMPERATURE_PRECISION);
      //delay(750/ (1 << (12-TEMPERATURE_PRECISION)));
    }
  }
  /* END TEMPERATURES SECTION */


}

void loop() {
  /* TEMPERATURES SECTION */
  float temperatures[Limit]; // declaring it here, once I know the dimension

  if(millis() - lastTempRequest >= conversionTime_DS18B20_sensors){
    for(byte index = 0; index < Limit; index++){
      temperatures[index] = sensors.getTempC(Thermometer[index]);
    }
    sensors.requestTemperatures();
    lastTempRequest = millis();
  }

  temperatureController.periodicRun(temperatures, Limit);

  bool heating = temperatureController.getOutputState();
  
  for(byte index = 0; index < Limit; index++){
    Serial.print("Temp ");
    Serial.print(index);
    Serial.print(": ");
    Serial.print(temperatures[index]);
    Serial.print(" ");
  }
  Serial.print("Heater State: ");
  Serial.print(heating);
  Serial.print(" ActualT: ");
  Serial.print(temperatureController.debug_getActualTemperature());
  Serial.print(" HS: ");
  Serial.print(temperatureController.debug_getHysteresisState());
  Serial.println("");
  
  /* END TEMPERATURES SECTION */

}
