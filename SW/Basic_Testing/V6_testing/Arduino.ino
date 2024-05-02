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

 https://lastminuteengineers.com/multiple-ds18b20-arduino-tutorial/ reading temperature by address
*/

#include <SoftwareSerial.h>
#include <avr/wdt.h>


#define SERIAL_SPEED 19200 
#define WATCHDOG_ENABLE true
#define DEFAULT_DEBOUNCE_TIME 25 //ms
#define ENABLE_SERIAL_PRINT_TO_ESP8266 true
#define ENABLE_SERIAL_PRINT_DEBUG false

/* TEMPERATURES SECTION */
// ricorda di mettere una alimentazione forte e indipendente per i sensori. Ha migliorato molto la stabilità della lettura.
#include <ProfiloLibrary.h>
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

temperatureController temperatureController;

bool automaticControl_var = false;
#define AUTOMATIC_CONTROL_CHECK_PIN 6

float higherHysteresisLimit, lowerHysteresisLimit;

bool mainHeater_var = false;
#define MAIN_HEATER_PIN 12

bool auxHeater_var = false;
#define AUX_HEATER_PIN 11

bool upperFan_var = false;
#define UPPER_FAN_PIN 10

bool lowerFan_var = false;
#define LOWER_FAN_PIN 9

float temperatures[3]; // declaring it here, once I know the dimension
int temperatureControlModality = 1; // default TEMPERATURE CONTROL MODALITY 1: actualTemperature = maxValue
/* END TEMPERATURES SECTION */


/* MOTORS SECTION */
#define STEPPER_MOTOR_SPEED_DEFAULT 10 //rpm

byte stepPin = 8;
byte directionPin = 7;
byte MS1 = 6;
byte MS2 = 5;
byte MS3 = 4;

float stepper_motor_speed = STEPPER_MOTOR_SPEED_DEFAULT;

stepperMotor eggsTurnerStepperMotor(stepPin, directionPin, MS1, MS2, MS3, 0.5);

bool move = false;
bool direction = false; 

bool stepperAutomaticControl_var = false;

#define LEFT_INDUCTOR_PIN 6
antiDebounceInput leftInductor_input(LEFT_INDUCTOR_PIN, DEFAULT_DEBOUNCE_TIME);

#define RIGHT_INDUCTOR_PIN 5
antiDebounceInput rightInductor_input(RIGHT_INDUCTOR_PIN, DEFAULT_DEBOUNCE_TIME);

// variabile per manual control
bool stepperMotor_moveForward_var = false;
bool stepperMotor_moveBackward_var = false;
bool stepperMotor_stop_var = false;

byte eggsTurnerState = 0;
bool turnEggs_cmd = false;

timer dummyTimer;
/* END MOTORS SECTION */

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
// handling float numbers
char bufferChar[35];
char fbuffChar[10];

#define TIME_PERIOD_SERIAL_COMMUNICATION_ARDUINO_TO_ESP8266 100 //ms
timer timerSerialToESP8266;


/* GENERAL */



void setup() {
  pinMode(stepPin, OUTPUT);
  digitalWrite(stepPin, LOW);

  pinMode(directionPin, OUTPUT);
  digitalWrite(directionPin, LOW);  

  pinMode(MS1, OUTPUT);
  digitalWrite(MS1, LOW);

  pinMode(MS2, OUTPUT);
  digitalWrite(MS2, LOW);

  pinMode(MS3, OUTPUT);
  digitalWrite(MS3, LOW);


  pinMode(MAIN_HEATER_PIN, OUTPUT);
  pinMode(AUX_HEATER_PIN, OUTPUT);
  pinMode(UPPER_FAN_PIN, OUTPUT);
  pinMode(LOWER_FAN_PIN, OUTPUT);
  pinMode(AUTOMATIC_CONTROL_CHECK_PIN, OUTPUT);

  pinMode(LEFT_INDUCTOR_PIN, INPUT);
  pinMode(RIGHT_INDUCTOR_PIN, INPUT);

  Serial.begin(SERIAL_SPEED);

  if(WATCHDOG_ENABLE){
    wdt_disable();
    wdt_enable(WDTO_2S);
  }
  
  serialFlush();
  delay(5);

  /* TEMPERATURES SECTION */
  higherHysteresisLimit = 26.0;
  lowerHysteresisLimit = 24.0;

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

  timerSerialToESP8266.setTimeToWait(TIME_PERIOD_SERIAL_COMMUNICATION_ARDUINO_TO_ESP8266);
  /* END TEMPERATURES SECTION */

  dummyTimer.setTimeToWait(10000); //DUMMY FOR TESTS - giro ogni 10 secondi.
  rightInductor_input.changePolarity(); // DUMMY con sensore NON balluff
}

void loop() {    
  /* TEMPERATURES SECTION */
  if(millis() - lastTempRequest >= (2*conversionTime_DS18B20_sensors)){
    for(byte index = 0; index < Limit; index++){
      temperatures[index] = sensors.getTempC(Thermometer[index]); // memorizza tutte le temperature che ci pensa l'oggetto temperatureController a gestirle
    }
	
    sensors.requestTemperatures();
    lastTempRequest = millis();
  }

  temperatureController.setTemperatureHysteresis(lowerHysteresisLimit, higherHysteresisLimit);
  temperatureController.setControlModality(temperatureControlModality); 
  temperatureController.periodicRun(temperatures, Limit);

  /*
  Serial.print("T1: ");
  Serial.print(temperatures[0]);
  Serial.print(" T2: ");
  Serial.print(temperatures[1]);
  Serial.print(" T3: ");
  Serial.print(temperatures[2]);
  Serial.print(" aT: ");
  Serial.println(temperatureController.debug_getActualTemperature());
  */
  if(automaticControl_var){
    mainHeater_var = temperatureController.getOutputState();
    auxHeater_var = false;
    digitalWrite(MAIN_HEATER_PIN, mainHeater_var);
    digitalWrite(UPPER_FAN_PIN, HIGH); 
    digitalWrite(LOWER_FAN_PIN, HIGH); 
    digitalWrite(AUTOMATIC_CONTROL_CHECK_PIN, HIGH);
  }
  else{
    digitalWrite(MAIN_HEATER_PIN, mainHeater_var);
    digitalWrite(AUX_HEATER_PIN, auxHeater_var);
    digitalWrite(UPPER_FAN_PIN, upperFan_var); 
    digitalWrite(LOWER_FAN_PIN, lowerFan_var);
    digitalWrite(AUTOMATIC_CONTROL_CHECK_PIN, LOW);
  }
  /* END TEMPERATURES SECTION */

  /* INDUCTOR INPUT SECTION */
  leftInductor_input.periodicRun();
  rightInductor_input.periodicRun();
  /* END INDUCTOR INPUT SECTION */
    
  /* STEPPER MOTOR SECTION */
  eggsTurnerStepperMotor.periodicRun();

  dummyTimer.periodicRun();
  if(stepperAutomaticControl_var){
    /* macchina a stati per la gestione della rotazione */
    dummyTimer.enable();
    
    turnEggs_cmd = dummyTimer.getOutputTriggerEdgeType();

    // direzione oraria = forward = verso l'induttore di destra (vedendo da dietro l'incubatrice)
    switch(eggsTurnerState){
      case 0: // ZEROING
        // lascio stato aperto per eventuale abilitazione della procedura di azzeramento da parte dell'operatore.
        eggsTurnerStepperMotor.moveBackward(STEPPER_MOTOR_SPEED_DEFAULT);
        eggsTurnerState = 1;
        break;
      case 1: // WAIT_TO_REACH_LEFT_SIDE_INDUCTOR
        if(leftInductor_input.getInputState()){
          // home position is reached - full left
          eggsTurnerStepperMotor.stopMotor();
          dummyTimer.reset();
          eggsTurnerState = 2;
        }
        break;
      case 2: // WAIT_FOR_TURN_EGGS_COMMAND_STATE --> will rotate from left to right (CW direction)
          if(turnEggs_cmd){
            eggsTurnerStepperMotor.moveForward(STEPPER_MOTOR_SPEED_DEFAULT);
            eggsTurnerState = 3;
          }
        break;
      case 3: // WAIT_TO_REACH_RIGHT_SIDE_INDUCTOR
          if(rightInductor_input.getInputState()){
            dummyTimer.reArm();
            eggsTurnerStepperMotor.stopMotor();
            eggsTurnerState = 4;
          }
        break;
      case 4: // WAIT_FOR_TURN_EGGS_COMMAND_STATE --> will rotate from right to left (CCW direction)
          if(turnEggs_cmd){
            eggsTurnerStepperMotor.moveBackward(STEPPER_MOTOR_SPEED_DEFAULT);
            eggsTurnerState = 5;
          }
        break;
      case 5: // WAIT_TO_REACH_LEFT_SIDE_INDUCTOR
          if(leftInductor_input.getInputState()){
            dummyTimer.reArm();
            eggsTurnerStepperMotor.stopMotor();
            eggsTurnerState = 2;
          }
        break;
      default:
        break;
    }
    if(ENABLE_SERIAL_PRINT_DEBUG){
      Serial.print(eggsTurnerState);
      Serial.print("  ");
      Serial.print(leftInductor_input.getInputState());
      Serial.print("  ");
      Serial.print(rightInductor_input.getInputState());
      Serial.println("");
    }
  }
  else{
    dummyTimer.disable();
    if(stepperMotor_moveForward_var){
      eggsTurnerStepperMotor.moveForward(STEPPER_MOTOR_SPEED_DEFAULT);
    }
    if(stepperMotor_moveBackward_var){
      eggsTurnerStepperMotor.moveBackward(STEPPER_MOTOR_SPEED_DEFAULT);
    }
    if(stepperMotor_stop_var){
      eggsTurnerStepperMotor.stopMotor();
    }

    //clear variables
    stepperMotor_moveForward_var = false;
    stepperMotor_moveBackward_var = false;
    stepperMotor_stop_var = false;
  }
  
  /* END STEPPER MOTOR SECTION */

  /* SERIAL COMMUNICATION FROM ARDUINO TO ESP8266 */
  timerSerialToESP8266.periodicRun();
  if(timerSerialToESP8266.getOutputTriggerEdgeType()){
    timerSerialToESP8266.reArm();
    // riempo i dati per la trasmissione
    listofDataToSend_numberOfData = 0; // ad ogni giro lo azzero    

    strcpy(bufferChar, "<t1, ");
    dtostrf( temperatures[0], 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(strcat(bufferChar, fbuffChar), ">"), '\0');
    listofDataToSend_numberOfData++;

    strcpy(bufferChar, "<t2, ");
    dtostrf( temperatures[1], 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(strcat(bufferChar, fbuffChar), ">"), '\0');
    listofDataToSend_numberOfData++; 

    strcpy(bufferChar, "<t3, ");
    dtostrf( temperatures[2], 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(strcat(bufferChar, fbuffChar), ">"), '\0');
    listofDataToSend_numberOfData++;

    strcpy(bufferChar, "<aT, "); // actualTemperature
    dtostrf(temperatureController.debug_getActualTemperature(), 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(strcat(bufferChar, fbuffChar), ">"), '\0');
    listofDataToSend_numberOfData++;
  
    strcpy(bufferChar, "<hHL, "); //higher Hysteresis Limit from Arduino
    dtostrf(higherHysteresisLimit, 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(strcat(bufferChar, fbuffChar), ">"), '\0');
    listofDataToSend_numberOfData++;

    strcpy(bufferChar, "<lHL, "); //lower Hysteresis Limit from Arduino
    dtostrf(lowerHysteresisLimit, 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(strcat(bufferChar, fbuffChar), ">"), '\0');
    listofDataToSend_numberOfData++;

    // feedback about mainHeater state
    listofDataToSend[listofDataToSend_numberOfData] = mainHeater_var ? "<mHO, 1>":"<mHO, 0>"; // converting bool to string
    listofDataToSend_numberOfData++;

    // feedback about auxHeater state
    listofDataToSend[listofDataToSend_numberOfData] = auxHeater_var ? "<aHO, 1>":"<aHO, 0>"; // converting bool to string
    listofDataToSend_numberOfData++;
    
    // feedback about TEMPERATURE CONTROL MODALITY value
    char bufferCharArray[25];	
    byte charCount = sprintf(bufferCharArray, "<TMP07, %d>", temperatureControlModality); //sprintf function returns the number of characters written to the array
    listofDataToSend[listofDataToSend_numberOfData] = bufferCharArray;
    listofDataToSend_numberOfData++;

    if(ENABLE_SERIAL_PRINT_TO_ESP8266){
      if(listofDataToSend_numberOfData > 0){
        Serial.print("#"); // SYMBOL TO START BOARDS TRANSMISSION
        for(byte i = 0; i < listofDataToSend_numberOfData; i++){
          Serial.print(listofDataToSend[i]); 
        }
        Serial.print("@"); // SYMBOL TO END BOARDS TRANSMISSION
      }
    }
  }
    
  
  

  // appena il buffer è pieno, mi blocco a leggerlo. Significa che ESP8266 ha pubblicato dei dati.
  if(Serial.available() > 0){ 
      numberOfCommandsFromBoard = readFromBoard(); // from ESP8266. It has @ as terminator character
      // Guardiamo che comandi ci sono arrivati
      for(byte j = 0; j < numberOfCommandsFromBoard; j++){
        String tempReceivedCommand = receivedCommands[j];
        //Serial.println(tempReceivedCommand);
        if(tempReceivedCommand.indexOf("STP01") >= 0){ //stepperMotorForwardOn
          stepperMotor_moveForward_var = true;
        }
        if(tempReceivedCommand.indexOf("STP02") >= 0){ //stepperMotorForwardOff
          stepperMotor_stop_var = true;
        }
        if(tempReceivedCommand.indexOf("STP03") >= 0){ //stepperMotorBackwardOn
          stepperMotor_moveBackward_var = true;
        }
        if(tempReceivedCommand.indexOf("STP04") >= 0){ //stepperMotorBackwardOff
          stepperMotor_stop_var = true;
        }
        if(tempReceivedCommand.indexOf("STP05") >= 0){ //stepperMotorAutomaticControlOn
          stepperAutomaticControl_var = true;
        }
        if(tempReceivedCommand.indexOf("STP06") >= 0){ //stepperMotorAutomaticControlOff
          stepperAutomaticControl_var = false;
        }
        if(tempReceivedCommand.indexOf("TMP01") >= 0){ //mainHeaterOn
          mainHeater_var = true;
        }
        if(tempReceivedCommand.indexOf("TMP02") >= 0){ //mainHeaterOff
          mainHeater_var = false;
        }
        if(tempReceivedCommand.indexOf("TMP03") >= 0){ //auxHeaterOn
          auxHeater_var = true;
        }
        if(tempReceivedCommand.indexOf("TMP04") >= 0){ //auxHeaterOff
          auxHeater_var = false;
        }
        if(tempReceivedCommand.indexOf("ACT01") >= 0){ //upperFanOn
          upperFan_var = true;
        }
        if(tempReceivedCommand.indexOf("ACT02") >= 0){ //upperFanOff
          upperFan_var = false;
        }
        if(tempReceivedCommand.indexOf("ACT03") >= 0){ //lowerFanOn
          lowerFan_var = true;
        }
        if(tempReceivedCommand.indexOf("ACT04") >= 0){ //lowerFanOff
          lowerFan_var = false;
        }
        if(tempReceivedCommand.indexOf("GNR01") >= 0){ //automaticControlOn
          automaticControl_var = true;
        }
        if(tempReceivedCommand.indexOf("GNR02") >= 0){ //automaticControlOff
          automaticControl_var = false;
        }
        if(tempReceivedCommand.indexOf("TMP05") >= 0){ //setting higherHysteresisLimit from ESP8266 HMTL page
          higherHysteresisLimit = getFloatFromString(tempReceivedCommand, ',');
        }
        if(tempReceivedCommand.indexOf("TMP06") >= 0){ //setting lowerHysteresisLimit from ESP8266 HMTL page
          lowerHysteresisLimit = getFloatFromString(tempReceivedCommand, ',');
        }
		    if(tempReceivedCommand.indexOf("TMP07") >= 0){ //setting TEMPERATURE CONTROL MODALITY from ESP8266 HMTL page
          // 1: actualTemperature = maxValue
          // 2: actualTemperature = meanValue
          temperatureControlModality = getIntFromString(tempReceivedCommand, ',');
        }
      }
  } 
  
  if(WATCHDOG_ENABLE){
    wdt_reset();
  }  
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
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}
