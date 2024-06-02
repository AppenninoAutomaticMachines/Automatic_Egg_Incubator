/* LIBRARIES */
#include <SoftwareSerial.h>
#include <avr/wdt.h>
#include <ProfiloLibrary.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Wire.h>
#include <RTClib.h>


/* PIN ARDUINO */
#define MAIN_HEATER_PIN 12
#define AUX_HEATER_PIN 11
#define UPPER_FAN_PIN 10
#define LOWER_FAN_PIN 9
#define STEPPPER_MOTOR_STEP_PIN 8
#define STEPPPER_MOTOR_DIRECTION_PIN 7
#define STEPPPER_MOTOR_MS1_PIN 6 
#define STEPPPER_MOTOR_MS2_PIN 5
#define STEPPPER_MOTOR_MS3_PIN 4
#define CYCLE_TOGGLE_PIN 3
#define CYCLE_TOGGLE_ANALOG0_PIN 14

#define LEFT_INDUCTOR_PIN 6
#define RIGHT_INDUCTOR_PIN 5
#define DHTPIN 4   //Pin a cui è connesso il sensore
#define ONE_WIRE_BUS 2

// A4 and A5 used for RTC module

#define SERIAL_SPEED 19200 
#define WATCHDOG_ENABLE true
#define DEFAULT_DEBOUNCE_TIME 25 //ms
#define ENABLE_SERIAL_PRINT_TO_ESP8266 true
#define ENABLE_SERIAL_PRINT_DEBUG false

/* TEMPERATURES SECTION */
// ricorda di mettere una alimentazione forte e indipendente per i sensori. Ha migliorato molto la stabilità della lettura.
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

temperatureController temperatureController;

bool automaticControl_var = false;

float higherHysteresisLimit, lowerHysteresisLimit;

bool mainHeater_var = false;
bool auxHeater_var = false;
bool upperFan_var = false;
bool lowerFan_var = false;


float temperatures[3]; // declaring it here, once I know the dimension
int temperatureControlModality = 1; // default TEMPERATURE CONTROL MODALITY 1: actualTemperature = maxValue

DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

// Define a char array to store the hexadecimal representation of the address
char addressCharArray[17]; // 16 characters for the address + 1 for null terminator

char comparisonAddress[] = "28FF888230180179"; // indirizzo del sensore di temperatura che uso per fare HUMIDITY

byte controlTemperatureIndex; // i sensori di temperatura sono 4, ma quelli effettivi per il controllo della T sono 3. Uno è per l'umidità. Serve un indice separato.
/* END TEMPERATURES SECTION */

float humiditySensor_temperature; // variabile buffer in cui, durante la lettura dei sensori DS18B20, metto quella del bulbo umido

float temp_sensor1, temp_sensor2, temp_sensor3;

//RECEIVING FROM ESP8266
#define MAX_NUMBER_OF_COMMANDS_FROM_BOARD 20
bool receivingDataFromBoard = false;
String receivedCommands[MAX_NUMBER_OF_COMMANDS_FROM_BOARD];
byte numberOfCommandsFromBoard;

// SENDING TO ESP8266
#define MAX_NUMBER_OF_COMMANDS_TO_BOARD 20
String listofDataToSend[MAX_NUMBER_OF_COMMANDS_TO_BOARD];
byte listofDataToSend_numberOfData = 0;

char bufferCharArray[25]; // buffer lo metto qui
// handling float numbers
char bufferChar[35];
char fbuffChar[10];

#define TIME_PERIOD_SERIAL_COMMUNICATION_ARDUINO_TO_ESP8266 100 //ms
timer timerSerialToESP8266;


/* GENERAL */
bool cycle_toggle_pin_var = false;
bool gotTemperatures = false;

/* variabile che viene messa a true quando viene richiesto al motore di andare.
  La usi ogni qualvolta devi interrompere qualcosa per dare spazio al motore.
*/
bool inhibit_stepperMotorRunning = false; 

bool ledCheck = false; // variabile che fa il check del led PIN 13. rimane falsa, quindi tiene accesa il led. Appena faccio un accesso da HTML, allora resetto il led.

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(7, INPUT);
  pinMode(8, INPUT);

  Serial.begin(SERIAL_SPEED);
  Serial.println("inizio");

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
  Serial.println(numberOfDevices);
  for(byte index = 0; index < Limit; index++){
    if(sensors.getAddress(Thermometer[index], index)){
      sensors.setResolution(Thermometer[index], TEMPERATURE_PRECISION);
      //delay(750/ (1 << (12-TEMPERATURE_PRECISION)));
    }
  }

  digitalWrite(LED_BUILTIN, HIGH); // all'avvio accendo il LED_BUILTIN. Durante il funzionamento lo resetto. Se ritorno e lo vedo acceso, significa che ha agito il watchdog.
  Serial.println("ciao");
}

void loop() {    
  /* TEMPERATURES SECTION */
  digitalWrite(CYCLE_TOGGLE_ANALOG0_PIN, HIGH);
  if(millis() - lastTempRequest >= (1000)){ // conversionTime_DS18B20_sensors refresh temperature ogni 1s
    controlTemperatureIndex = 0;
    for(byte index = 0; index < Limit; index++){
      // Call the function to convert the device address to a char array
      addressToCharArray(Thermometer[index], addressCharArray);
      if(strcmp(addressCharArray, comparisonAddress) == 0){
        // calcolo dell'umidità
        humiditySensor_temperature = sensors.getTempC(Thermometer[index]);
      }
      else{
        // gli altri li possiamo streammare verso html
        temperatures[controlTemperatureIndex] = sensors.getTempC(Thermometer[index]); // memorizza tutte le temperature che ci pensa l'oggetto temperatureController a gestirle
        controlTemperatureIndex += 1;
      }     
      
    }
    gotTemperatures = true;
  
    sensors.requestTemperatures();
    lastTempRequest = millis();
  } 
  digitalWrite(CYCLE_TOGGLE_ANALOG0_PIN, LOW);

  listofDataToSend_numberOfData = 0;
  if(gotTemperatures){    
    strcpy(bufferChar, "<TMP01,");
    dtostrf( temperatures[0], 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(bufferChar, fbuffChar), ">");
    listofDataToSend_numberOfData++;

    strcpy(bufferChar, "<TMP02,");
    dtostrf( temperatures[1], 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(bufferChar, fbuffChar), ">");
    listofDataToSend_numberOfData++; 

    strcpy(bufferChar, "<TMP03,");
    dtostrf( temperatures[2], 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(bufferChar, fbuffChar), ">");
    listofDataToSend_numberOfData++;
    gotTemperatures = false;
  }
  /*
  if(digitalRead(7)){
    listofDataToSend[listofDataToSend_numberOfData] = "<pin7,HIGH>";
    listofDataToSend_numberOfData++;
  }
  else{
    listofDataToSend[listofDataToSend_numberOfData] = "<pin7,LOW>";
    listofDataToSend_numberOfData++;
  }

  if(digitalRead(8)){
    listofDataToSend[listofDataToSend_numberOfData] = "<pin8,HIGH>";
    listofDataToSend_numberOfData++;
  }
  else{
    listofDataToSend[listofDataToSend_numberOfData] = "<pin8,LOW>";
    listofDataToSend_numberOfData++;
  }
  */

  if(listofDataToSend_numberOfData > 0){
    Serial.print('@'); // SYMBOL TO END BOARDS TRANSMISSION
    for(byte i = 0; i < listofDataToSend_numberOfData; i++){
      Serial.print(listofDataToSend[i]); 
    }
    Serial.print('#'); // SYMBOL TO END BOARDS TRANSMISSION
  }
  
  



  /* END TEMPERATURES SECTION */

  digitalWrite(CYCLE_TOGGLE_PIN, cycle_toggle_pin_var);
  cycle_toggle_pin_var = !cycle_toggle_pin_var;
  
}

int readFromBoard(){ // returns the number of commands received
  receivingDataFromBoard = true;
  byte rcIndex = 0;
  char bufferChar[30];
  bool saving = false;
  bool enableReading = false;
  byte receivedCommandsIndex = 0;

  while(receivingDataFromBoard){
    // qui è bloccante...assumo che prima o poi arduino pubblichi
    if(Serial.available() > 0){ // no while, perché potrei avere un attimo il buffer vuoto...senza aver ancora ricevuto il terminatore
      char rc = Serial.read(); 

      if(rc == '#'){ // // SYMBOL TO START TRANSMISSION (tutto quello che c'era prima era roba spuria che ho pulito)
        enableReading = true;
      }

      if(enableReading){
        if(rc == '<'){ // SYMBOL TO START MESSAGE
          saving = true;
        }
        else if(rc == '>'){ // SYMBOL TO END MESSAGE
          bufferChar[rcIndex] = '\0';
          receivedCommands[receivedCommandsIndex] = bufferChar;
          receivedCommandsIndex ++;
          rcIndex = 0;
          saving = false;// starting char
        }
        else if(rc == '@'){ // SYMBOL TO END BOARDS TRANSMISSION
          receivingDataFromBoard = false; // buffer vuoto, vado avanti
          //enableReading = false
        }
        else{
          if(saving){
            bufferChar[rcIndex] = rc;
            rcIndex ++;
          }
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

  return string.substring(index, string.length()).toFloat();
}

int getIntFromString(String string, char divider){
  int index;
  for(byte i =0; i < string.length(); i++) {
    char c = string[i];
    
    if(c == divider){
      index = i + 2;
      break;
    }
  }

  return string.substring(index, string.length()).toInt();
}

void serialFlush(){
  while(Serial.available() > 0){
    char t = Serial.read();
  }
}

// Function to convert a byte to its hexadecimal representation
void byteToHex(uint8_t byteValue, char *hexValue) {
  uint8_t highNibble = byteValue >> 4;
  uint8_t lowNibble = byteValue & 0x0F;
  hexValue[0] = highNibble < 10 ? '0' + highNibble : 'A' + (highNibble - 10);
  hexValue[1] = lowNibble < 10 ? '0' + lowNibble : 'A' + (lowNibble - 10);
}

// Function to print a device address
void addressToCharArray(DeviceAddress deviceAddress, char *charArray) {
  for (uint8_t i = 0; i < 8; i++) {
    byteToHex(deviceAddress[i], &charArray[i * 2]);
  }
  // Add null terminator at the end of the char array
  charArray[16] = '\0';
}

int computeTimeDifference_inMinutes(DateTime currentTime, DateTime lastTrigger){
  // unixtime --> is a standard way of representing time as a single numeric value
  unsigned long diffSeconds = currentTime.unixtime() - lastTrigger.unixtime();
  int differenceInMinutes = (diffSeconds % 3600) / 60;

  return differenceInMinutes;
}

int computeTimeDifference_inSeconds(DateTime currentTime, DateTime lastTrigger){
  // unixtime --> is a standard way of representing time as a single numeric value
  unsigned long diffSeconds = currentTime.unixtime() - lastTrigger.unixtime();

  return diffSeconds;
}
