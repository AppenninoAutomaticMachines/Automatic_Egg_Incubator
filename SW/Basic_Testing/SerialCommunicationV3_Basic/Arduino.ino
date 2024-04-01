/*
  Set the DIP switch to Mode 1 (SW3 and SW4 are On )with power removed. Restore power and upload the Arduino Sketch “ArduinoESPCommunication
  Remove power and set the DIP switch to Mode 2 (SW5, SW6, SW7 are On)
  Change Arduino IDE to ESP8266 programming mode and upload the sketch “ESPRecieveCommunication”
  Remove power and set the dip switch to Mode 4 (SW1 and SW2 are On)
  Restore power
  Pressing the push button will cause the ATMega328 built in LED to flash and the LED connected to the ESP8266 will come on.
  Pressing the push button again will flash the built in LED and turn off the LED connected to the ESP8266

 Arduino UNO modalità programmazione: Arduino Uno normale Mode 1 (SW3 and SW4 are On )
 Arduino UNO modalità funzionamento + comunicazione seriale con ESP8266: Arduino Uno normale Mode 4 (SW1 and SW2 are On)
*/

#include <SoftwareSerial.h>

/* TEMPERATURES SECTION */
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

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
/* END TEMPERATURES SECTION */

/* MOTORS SECTION */
byte directionPin = 7;
byte stepPin = 8;

bool move = false;
bool direction = false; 
/* END MOTORS SECTION */

float temp_sensor1, temp_sensor2, temp_sensor3;
bool ESP8266_superSlowCode_isExecuting = false;

//RECEIVING FROM ESP8266
#define MAX_NUMBER_OF_COMMANDS_FROM_BOARD 20
bool receivingDataFromBoard = false;
String receivedCommands[MAX_NUMBER_OF_COMMANDS_FROM_BOARD];
byte numberOfCommandsFromBoard;

// SENDING TO ESP8266
#define MAX_NUMBER_OF_COMMANDS_TO_BOARD 20
String listofDataToSend[MAX_NUMBER_OF_COMMANDS_TO_BOARD];
byte listofDataToSend_numberOfData = 0;
// handling float numbers
char bufferChar[30];
char fbuffChar[10];

void setup() {
  pinMode(directionPin, OUTPUT);
  digitalWrite(directionPin, LOW);

  pinMode(stepPin, OUTPUT);
  digitalWrite(stepPin, LOW);

  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(9, OUTPUT);

  Serial.begin(9600); // che scrive veloce verso ESP8266
  
  serialFlush();
  delay(5);

  /* TEMPERATURES SECTION */
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
}

void loop() {    
  // NORMAL CODE
  if(millis() - lastTempRequest >= conversionTime_DS18B20_sensors){
    for(byte index = 0; index < Limit; index++){
      float tempC = sensors.getTempC(Thermometer[index]);
      if (tempC == DEVICE_DISCONNECTED) {
        ;//Serial.print(F("Error getting temperature"));
      } 
      else {
        //Serial.print(tempC);
        if(index == 0){
          temp_sensor1 = tempC;
        }
        else if(index == 1){
          temp_sensor2 = tempC;
        }
        else if(index == 2){
          temp_sensor3 = tempC;
        }
      }
    }
    sensors.requestTemperatures();
    lastTempRequest = millis();
  }
    
  if(move){
    digitalWrite(stepPin, HIGH);
    delay(10);
    digitalWrite(stepPin, LOW);
  }
  else{
    digitalWrite(stepPin, LOW);
  }
  

  // riempo i dati per la trasmissione
  listofDataToSend_numberOfData = 0; // ad ogni giro lo azzero    

  strcpy(bufferChar, "<temp1, ");
  dtostrf(temp_sensor1, 1, 1, fbuffChar); 
  listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(strcat(bufferChar, fbuffChar), ">"), '\0');
  listofDataToSend_numberOfData++;

  strcpy(bufferChar, "<temp2, ");
  dtostrf(temp_sensor2, 1, 1, fbuffChar); 
  listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(strcat(bufferChar, fbuffChar), ">"), '\0');
  listofDataToSend_numberOfData++;

  strcpy(bufferChar, "<temp3, ");
  dtostrf(temp_sensor3, 1, 1, fbuffChar); 
  listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(strcat(bufferChar, fbuffChar), ">"), '\0');
  listofDataToSend_numberOfData++;

  // printing over serial buffer:
  serialFlush();
  delay(1);
  
  for(byte i = 0; i < listofDataToSend_numberOfData; i++){
    Serial.print(listofDataToSend[i]);
  }
  Serial.print("#"); // end transmission for Arduino
  delay(1);
  

  // appena il buffer è pieno, mi blocco a leggerlo. Significa che ESP8266 ha pubblicato dei dati.
  if(Serial.available() > 0){ 
      numberOfCommandsFromBoard = readFromBoard('@'); // from ESP8266. It has @ as terminator character
      ESP8266_superSlowCode_isExecuting = false;

      // Guardiamo che comandi ci sono arrivati
      for(byte j = 0; j < numberOfCommandsFromBoard; j++){
        String tempReceivedCommand = receivedCommands[j];
        if(tempReceivedCommand.indexOf("stepperMotorForwardOn") >= 0){
          move = true;
          digitalWrite(directionPin, HIGH);
        }
        if(tempReceivedCommand.indexOf("stepperMotorForwardOff") >= 0){
          move = false;
        }
        if(tempReceivedCommand.indexOf("stepperMotorBackwardOn") >= 0){
          move = true;
          digitalWrite(directionPin, LOW);
        }
        if(tempReceivedCommand.indexOf("stepperMotorBackwardOff") >= 0){
          move = false;
        }
        if(tempReceivedCommand.indexOf("mainHeaterOn") >= 0){
          digitalWrite(12, HIGH);
        }
        if(tempReceivedCommand.indexOf("mainHeaterOff") >= 0){
          digitalWrite(12, LOW);
        }
        if(tempReceivedCommand.indexOf("auxHeaterOn") >= 0){
          digitalWrite(11, HIGH);
        }
        if(tempReceivedCommand.indexOf("auxHeaterOff") >= 0){
          digitalWrite(11, LOW);
        }
        if(tempReceivedCommand.indexOf("upperFanOn") >= 0){
          digitalWrite(10, HIGH);
        }
        if(tempReceivedCommand.indexOf("upperFanOff") >= 0){
          digitalWrite(10, LOW);
        }
        if(tempReceivedCommand.indexOf("lowerFanOn") >= 0){
          digitalWrite(9, HIGH);
        }
        if(tempReceivedCommand.indexOf("lowerFanOff") >= 0){
          digitalWrite(9, LOW);
        }
      }
  } 
  
}

int readFromBoard(char endOfCommunication){ // returns the number of commands received
  receivingDataFromBoard = true;
  byte rcIndex = 0;
  char bufferChar[30];
  bool saving = false;
  byte receivedCommandsIndex = 0;
  while(receivingDataFromBoard){
    if(Serial.available() > 0){ // no while, perché potrei avere un attimo il buffer vuoto...senza aver ancora ricevuto il terminatore
      char rc = Serial.read(); 

      if(rc == '<'){ // starting char
        saving = true;
      }
      else if(rc == '>'){ // ending char
        bufferChar[rcIndex] = '\0';
        receivedCommands[receivedCommandsIndex] = bufferChar;
        receivedCommandsIndex ++;
        rcIndex = 0;
        saving = false;// starting char
      }
      else if(rc == endOfCommunication){ // ending communication from board
        receivingDataFromBoard = false; // buffer vuoto, vado avanti
      }
      else{
        if(saving){
          bufferChar[rcIndex] = rc;
          rcIndex ++;
        }
      }
    }
  }
  return receivedCommandsIndex;
}

float getFloatFromString(String string, char divider){
  int index;
  for(byte i =0; i < string.length(); i++) {
    char c = string[i];
    
    if(c == divider){
      index = i + 2;
      break;
    }
  }

  return string.substring(index, (string.length()-1)).toFloat();
}

void serialFlush(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}
