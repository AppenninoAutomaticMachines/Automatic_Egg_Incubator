// https://forum.arduino.cc/t/what-doing-the-requesttemperatures-in-the-dallastemperature-library/185950
#include <OneWire.h>
#include <DallasTemperature.h>

// Setup a oneWire instance to communicate with any OneWire devices
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

#define TEMPERATURE_PRECISION 12 // DS18B20 digital termometer provides 9-bit to 12-bit Celsius temperature measurements
#define MAX_SENSORS 10
DeviceAddress Thermometer[MAX_SENSORS];
byte numberOfDevices;
byte Limit;

#if !defined(DEVICE_DISCONNECTED)
#define DEVICE_DISCONNECTED -127
#endif

const byte pin_upperFan = 10;
const byte pin_lowerFan = 9;
const byte pin_mainHeater = 8;
const byte pin_auxiliaryHeater = 7;

void setup(void){
  pinMode(pin_upperFan, OUTPUT);
  pinMode(pin_lowerFan, OUTPUT);
  pinMode(pin_mainHeater, OUTPUT);
  pinMode(pin_auxiliaryHeater, OUTPUT);

  Serial.begin(9600);
  // delay for terminal
  delay(2000);
  sensors.begin();

  Serial.print(F("Locating devices..."));
  numberOfDevices = sensors.getDeviceCount();
  Serial.print(F("Found "));
  Serial.print(numberOfDevices, DEC);
  Serial.println(F(" devices."));

  Serial.print(F("Parasite power is: ")); 
  if (sensors.isParasitePowerMode()){
    // parasitePower is deriving power directly from the data line
    Serial.println(F("ON"));
  }
  else{
    Serial.println(F("OFF"));
  };
  if (numberOfDevices > MAX_SENSORS){
    Limit = MAX_SENSORS;
  }
  else{
    Limit = numberOfDevices;
  };
  for(byte index=0; index<Limit; index++){
    if(sensors.getAddress(Thermometer[index], index)){
      Serial.print(F("Found device "));
      Serial.print(index, DEC);
      Serial.print(F(" with address: "));
      printAddress(Thermometer[index]);
      Serial.println();

      Serial.print(F("Setting resolution to "));
      Serial.println(TEMPERATURE_PRECISION,DEC);
      sensors.setResolution(Thermometer[index], TEMPERATURE_PRECISION);
      delay(750/ (1 << (12-TEMPERATURE_PRECISION))); 
      Serial.print(F("Resolution actually set to: "));
      Serial.print(sensors.getResolution(Thermometer[index]), DEC); 
      Serial.println();
    }
    else{
      Serial.print(F("Found ghost device at "));
      Serial.print(index, DEC);
      Serial.print(F(" but could not detect address. Check power and cabling"));
    };
  };

  Serial.println(F("Getting temperatures..."));
}

float printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC == DEVICE_DISCONNECTED) {
    Serial.print(F("Error getting temperature"));
  } 
  else {
    Serial.print(tempC);
    /*
    Serial.print(F(" F: "));
     Serial.print(DallasTemperature::toFahrenheit(tempC));
     */
  };
  return tempC;
};

void loop(void)
{ 
  float tempC;
  sensors.requestTemperatures(); //can be omitted?
  
  /*  
  for(byte index=0; index<Limit; index++){
    Serial.print(F("Temperature "));
    printMAC(Thermometer[index]);
    Serial.print(F(" : "));
    tempC = printTemperature(Thermometer[index]);
    Serial.println(F(" \xB0\x43"));
  }
  */
  
  for(byte index=0; index<Limit; index++){
    tempC = printTemperature(Thermometer[index]);
    Serial.print("  ");
  }

  Serial.print(" ");
  
  int incomingByte;
  int value;
  // Reading commands:
  if(Serial.available() > 0){
    incomingByte = Serial.read();
  }

  if(incomingByte == '0'){ // accensione ventola superiore
    digitalWrite(pin_upperFan, HIGH);
    Serial.print(" Accensione ventola superiore");
  }

  if(incomingByte == '1'){ // spegnimento ventola superiore
    digitalWrite(pin_upperFan, LOW);
    Serial.print(" Spegnimento ventola superiore");
  }

  if(incomingByte == '2'){ // accensione ventola inferiore
    digitalWrite(pin_lowerFan, HIGH);
    Serial.print(" Accensione ventola inferiore");
  }

  if(incomingByte == '3'){ // spegnimento ventola inferiore
    digitalWrite(pin_lowerFan, LOW);
    Serial.print(" Spegnimento ventola inferiore");
  }

  if(incomingByte == '4'){ // accensione main heater
    digitalWrite(pin_mainHeater, HIGH);
    Serial.print(" Accensione main heater");
  }

  if(incomingByte == '5'){ // spegnimento main heater
    digitalWrite(pin_mainHeater, LOW);
    Serial.print(" Spegnimento main heater");
  }

  if(incomingByte == '6'){ // accensione auxiliary heater
    digitalWrite(pin_auxiliaryHeater, HIGH);
    Serial.print(" Accensione auxiliary heater");
  }

  if(incomingByte == '7'){ // spegnimento auxiliary heater
    digitalWrite(pin_auxiliaryHeater, LOW);
    Serial.print(" Spegnimento auxiliary heater");
  }

  Serial.println();

};

void printAddress(DeviceAddress deviceAddress){
  Serial.print(F("{ "));
  for (uint8_t i = 0; i < 8; i++){
    // zero pad the address if necessary
    Serial.print(F("0x"));
    if (deviceAddress[i] < 16) {Serial.print("0");};
    Serial.print(deviceAddress[i], HEX);
    if (i<7) {Serial.print(F(", "));};
  };
  Serial.print(F(" }"));
};

void printMAC(DeviceAddress deviceAddress){
  for (uint8_t i = 0; i < 8; i++){
    if (deviceAddress[i] < 16) {Serial.print("0");};
    Serial.print(deviceAddress[i], HEX);
    if (i<7) {Serial.print("-");};
  };
};
