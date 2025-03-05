/*
  Questo codice prende le temperature e le umidità e li impacchetta correttametne in trasmissione come richeisto da raspberry.
  Infine, dipendentemente da cosa succede (interruttori) manda a rpy altre info, per esempio carter aperto
  oppure motore che raggiunge le posizioni limite. Provo a fare tutto in modo asincrono: mando solo quando ne ho bisogno.
*/
/* LIBRARIES */
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <ProfiloLibrary.h>


/* General CONSTANTS */
#define SERIAL_SPEED 19200
#define ENABLE_DEVICE_ORDERING false // se true devi mettere gli indirizzi nell'ordine che vuoi, se false il vettore delle temperature non è ordinato in base agli indirizzi
#define NUMBER_OF_TEMPERATURES_SENSORS_ON_ONE_WIRE_BUS_2 4 //sensori di temperatura
#define DEFAULT_DEBOUNCE_TIME 25 //ms

/* PIN ARDUINO */
#define ONE_WIRE_BUS_2 2
#define TEMPERATURE_PRECISION 9 // DS18B20 digital termometer provides 9-bit to 12-bit Celsius temperature measurements
#define LED_12 12
#define LED_11 11
#define CCW_INDUCTOR_PIN 10 // induttore finecorsa SINISTRO (vista posteriore)
#define CW_INDUCTOR_PIN 9 // induttore finecorsa DESTRO (vista posteriore)

#define STEPPPER_MOTOR_STEP_PIN 8
#define STEPPPER_MOTOR_DIRECTION_PIN 7
#define STEPPPER_MOTOR_MS1_PIN 6 
#define STEPPPER_MOTOR_MS2_PIN 5
#define STEPPPER_MOTOR_MS3_PIN 4

/* TEMPERATURES SECTION */
// NO STAR/RING connection, only ONE SINGLE WIRE UTP cabme (unshielded twisted pair)
OneWire oneWire(ONE_WIRE_BUS_2);

DallasTemperature sensors(&oneWire);

#define TEMPERATURE_IDENTIFICATION_PROCEDURE true // se a true visualizzazione nel monitor seriale della temperatura e del sensore associato
DeviceAddress Thermometer[NUMBER_OF_TEMPERATURES_SENSORS_ON_ONE_WIRE_BUS_2];
byte numberOfDevices;
byte Limit;
unsigned long conversionTime_DS18B20_sensors; // in millis
unsigned long lastTempRequest;

#if !defined(DEVICE_DISCONNECTED)
#define DEVICE_DISCONNECTED -127
#endif

#define DEVICE_ERROR 85

/* Temperature sensors - addresses. LOWEST number = HIGHEST sensor, then follows the decreasing order */
// RICORDA LA VARIABILE ENABLE_DEVICE_ORDERING
char temperatureSensor_address0[] = "28FF640E7213DCBE";
char temperatureSensor_address1[] = "28FF640E7C2E42E0";
char temperatureSensor_address2[] = "28FF640E7F7492C3";
char temperatureSensor_address3[] = "28FF640E7F489F5E";

/* Temperature Diagnostic */
unsigned int deviceDisconnected[NUMBER_OF_TEMPERATURES_SENSORS_ON_ONE_WIRE_BUS_2];
unsigned int deviceError[NUMBER_OF_TEMPERATURES_SENSORS_ON_ONE_WIRE_BUS_2];


float temperatures[4]; 
float marginFactor = 1.2; // fattore moltiplicativo per aspettare un po' più di delay.
byte controlTemperatureIndex;
bool gotTemperatures;

byte orderedIndex;

DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

// Define a char array to store the hexadecimal representation of the address
char addressCharArray[17]; // 16 characters for the address + 1 for null terminator

/* END TEMPERATURES SECTION */


float temp_sensor1, temp_sensor2, temp_sensor3;

unsigned long startGetTemperatures, endGetTemperatures;

/* CCW_LS */
antiDebounceInput ccw_inductor_input(CCW_INDUCTOR_PIN, DEFAULT_DEBOUNCE_TIME);
trigger ccw_trigger;

/* CW_LS */
antiDebounceInput cw_inductor_input(CW_INDUCTOR_PIN, DEFAULT_DEBOUNCE_TIME);
trigger cw_trigger;


/* MOTORS SECTION */
#define STEPPER_MOTOR_SPEED_DEFAULT 5 //rpm  opportuno stare sotto i 30rpm, perché il tempo ciclo di arduino prende un po' troppo.

float stepper_motor_speed = STEPPER_MOTOR_SPEED_DEFAULT;

stepperMotor eggsTurnerStepperMotor(STEPPPER_MOTOR_STEP_PIN, STEPPPER_MOTOR_DIRECTION_PIN, STEPPPER_MOTOR_MS1_PIN, STEPPPER_MOTOR_MS2_PIN, STEPPPER_MOTOR_MS3_PIN, 1.8);

bool move = false;
bool direction = false; 

// variabile per manual control
bool stepperMotor_moveCCW_automatic_var = false;
bool stepperMotor_moveCW_automatic_var = false;
bool stepperMotor_stop_automatic_var = false;

byte eggsTurnerState = 0;
bool stepperAutomaticControl_var = true;

/* END MOTORS SECTION */

// SENDING TO RPY
#define MAX_NUMBER_OF_COMMANDS_TO_BOARD 20
bool receivingDataFromBoard = false;
String listofDataToSend[MAX_NUMBER_OF_COMMANDS_TO_BOARD];
byte listofDataToSend_numberOfData = 0;

char bufferCharArray[25]; // buffer lo metto qui
// handling float numbers
char bufferChar[35];
char fbuffChar[10];

String receivedCommands[20];


void setup() {
  pinMode(LED_12, OUTPUT);
  pinMode(LED_11, OUTPUT);
  pinMode(CCW_INDUCTOR_PIN, INPUT);
  pinMode(CW_INDUCTOR_PIN, INPUT);

  pinMode(STEPPPER_MOTOR_STEP_PIN, OUTPUT);
  digitalWrite(STEPPPER_MOTOR_STEP_PIN, LOW);

  pinMode(STEPPPER_MOTOR_DIRECTION_PIN, OUTPUT);
  digitalWrite(STEPPPER_MOTOR_DIRECTION_PIN, LOW);  

  pinMode(STEPPPER_MOTOR_MS1_PIN, OUTPUT);
  digitalWrite(STEPPPER_MOTOR_MS1_PIN, LOW);

  pinMode(STEPPPER_MOTOR_MS2_PIN, OUTPUT);
  digitalWrite(STEPPPER_MOTOR_MS2_PIN, LOW);

  pinMode(STEPPPER_MOTOR_MS3_PIN, OUTPUT);
  digitalWrite(STEPPPER_MOTOR_MS3_PIN, LOW);

  pinMode(CCW_INDUCTOR_PIN, INPUT);
  pinMode(CW_INDUCTOR_PIN, INPUT);

  Serial.begin(SERIAL_SPEED);
  //Serial.println("Starting");

  sensors.begin();

  // locate devices on the bus
  //Serial.print("Locating devices...");
  //Serial.print("Found ");

  numberOfDevices = sensors.getDeviceCount();
  /*
  if(numberOfDevices != NUMBER_OF_TEMPERATURES_SENSORS_ON_ONE_WIRE_BUS_2){
    Serial.println("Not found the correct number of devices on the bus.");
    Serial.print("Expected: ");
    Serial.print(NUMBER_OF_TEMPERATURES_SENSORS_ON_ONE_WIRE_BUS_2);
    Serial.print("  Found: ");
    Serial.println(numberOfDevices);
  }
  */

  //Serial.print(numberOfDevices, DEC);
  //Serial.println(" devices.");

  // report parasite power requirements
  /*
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  */

  sensors.setWaitForConversion(false); // quando richiedi le temperature requestTemperatures() la libreria NON aspetta il delay adeguato, quidni devi aspettarlo tu.
  sensors.requestTemperatures(); // send command to all the sensors for temperature conversion.
  lastTempRequest = millis(); 
  conversionTime_DS18B20_sensors = 750 / (1 << (12 - TEMPERATURE_PRECISION));  // res in {9,10,11,12}

  orderedIndex = 0;
  for(uint8_t index = 0; index < numberOfDevices; index++){
    if(sensors.getAddress(tempDeviceAddress, index)){ // fetch dell'indirizzo

      if(ENABLE_DEVICE_ORDERING){
        addressToCharArray(tempDeviceAddress, addressCharArray); // indirizzo convertito
        
        // ora ordino il vettore dei sensori.
        if(strcmp(addressCharArray, temperatureSensor_address0) == 0){ // returns 0 when the two strings are identical
          sensors.getAddress(Thermometer[0], index);
        }
        if(strcmp(addressCharArray, temperatureSensor_address1) == 0){ 
          sensors.getAddress(Thermometer[1], index);
        }
        if(strcmp(addressCharArray, temperatureSensor_address2) == 0){ 
          sensors.getAddress(Thermometer[2], index);
        }
        if(strcmp(addressCharArray, temperatureSensor_address3) == 0){ 
          sensors.getAddress(Thermometer[3], index);
        }
      }
      else{
        // riempo il vettore di temperature senza alcun ordine topologico.
        sensors.getAddress(Thermometer[index], index);
      }
        

      // initializing arrays - l'azzeramento posso farlo senza posizioni, non importa, tanto è tutto a 0.
      deviceDisconnected[index] = 0;
      deviceError[index] = 0;

      delay(5);
    }
  }

  //Serial.println("Setup finished");
}

void loop() {    
  // RECEIVING FROM RPI
  if(Serial.available() > 0){ 
    int numberOfCommandsFromBoard = readFromBoard(); // from ESP8266. It has @ as terminator character
    // Guardiamo che comandi ci sono arrivati
    for(byte j = 0; j < numberOfCommandsFromBoard; j++){
      String tempReceivedCommand = receivedCommands[j];
      Serial.println(tempReceivedCommand);
      if(tempReceivedCommand.indexOf("HTR01") >= 0  &&  tempReceivedCommand.indexOf("True") >= 0 ){ //stepperMotorForwardOn
        digitalWrite(LED_12, HIGH);
        //Serial.println("ON");
      }
      if(tempReceivedCommand.indexOf("HTR01") >= 0 && tempReceivedCommand.indexOf("False") >= 0 ){ //stepperMotorForwardOff
        digitalWrite(LED_12, LOW);
        //Serial.println("OFF");
      }
      if(tempReceivedCommand.indexOf("HUMER01") >= 0  &&  tempReceivedCommand.indexOf("True") >= 0 ){ //stepperMotorForwardOn
        digitalWrite(LED_11, HIGH);
        //Serial.println("ON");
      }
      if(tempReceivedCommand.indexOf("HUMER01") >= 0 && tempReceivedCommand.indexOf("False") >= 0 ){ //stepperMotorForwardOff
        digitalWrite(LED_11, LOW);
        //Serial.println("OFF");
      }
      if(tempReceivedCommand.indexOf("STPR01") >= 0 && tempReceivedCommand.indexOf("FTCCW") >= 0 ){ //full_turn_counter_clock_wise 
        stepperMotor_moveCCW_automatic_var = true;
      }
      if(tempReceivedCommand.indexOf("STPR01") >= 0 && tempReceivedCommand.indexOf("FTCW") >= 0 ){ //full_turn_clock_wise 
        stepperMotor_moveCW_automatic_var = true;
      }
      if(tempReceivedCommand.indexOf("STPR01") >= 0 && tempReceivedCommand.indexOf("STOP") >= 0 ){ //stop
        stepperMotor_stop_automatic_var = true;
      }
    }
  }
  delay(50);


  listofDataToSend_numberOfData = 0;

  /* TEMPERATURES SECTION */
  /*
    - Attesa di un delay sufficiente per la conversione di temperatura fatta simultaneamente da tutti i sensori di temperatura.
    - Una volta passato il tempo, iterativamente, chiedo a tutti i sensori la temperatura.

  */
  if(millis() - lastTempRequest >= (conversionTime_DS18B20_sensors * marginFactor)){
    startGetTemperatures = millis(); // per calcolare quanto tempo impiego a fetchare tutte le temperature dai sensori.
    for(uint8_t index = 0; index < numberOfDevices; index++){
      /* Memorize all temperatures in an ordered array */
      temperatures[index] = sensors.getTempC(Thermometer[index]);

      if(temperatures[index] <= DEVICE_DISCONNECTED){
        deviceDisconnected[index] ++;
      }
      if(temperatures[index] >= DEVICE_ERROR){
        deviceError[index] ++;
      }

      delay(1);       
    }
    gotTemperatures = true;
  
    sensors.requestTemperatures();
    lastTempRequest = millis();
    endGetTemperatures = millis();
  }   
  /* END TEMPERATURES SECTION */

  /* INDUCTOR INPUT SECTION */
  ccw_inductor_input.periodicRun();
  cw_inductor_input.periodicRun();
  /* END INDUCTOR INPUT SECTION */

  ccw_trigger.periodicRun(ccw_inductor_input.getInputState());
  cw_trigger.periodicRun(cw_inductor_input.getInputState());

  if(ccw_trigger.catchRisingEdge()){
    listofDataToSend[listofDataToSend_numberOfData] = "<IND_CCW, 1>"; // converting bool to string - "<IND_CW, 0>"
    listofDataToSend_numberOfData++;
  }

  if(cw_trigger.catchRisingEdge()){
    listofDataToSend[listofDataToSend_numberOfData] = "<IND_CW, 1>"; // converting bool to string - "<IND_CW, 0>"
    listofDataToSend_numberOfData++;
  }

  /* STEPPER MOTOR CONTROL SECTION */
  eggsTurnerStepperMotor.periodicRun();

  if(stepperAutomaticControl_var){
    // direzione oraria = forward = clock wise rotation direction (vista posteriore incubatrice)
    switch(eggsTurnerState){
      case 0: // ZEROING
        // lascio stato aperto per eventuale abilitazione della procedura di azzeramento da parte dell'operatore.
        if(ccw_inductor_input.getInputState()){ // se leggo già induttore, inutile comandare un bw. Sono già azzerato.  Devo però notificarlo a RPI!
          // home position is reached - full left
          listofDataToSend[listofDataToSend_numberOfData] = "<IND_CCW, 1>"; // converting bool to string - "<IND_CW, 0>"
          listofDataToSend_numberOfData++;

          eggsTurnerStepperMotor.stopMotor();       
          eggsTurnerState = 2;
        }
        else if(cw_inductor_input.getInputState()){ // se leggo già induttore. Sono già azzerato.  Devo però notificarlo a RPI!
          // home position is reached - full right
          listofDataToSend[listofDataToSend_numberOfData] = "<IND_CW, 1>"; // converting bool to string - "<IND_CW, 0>"
          listofDataToSend_numberOfData++;
          
          eggsTurnerStepperMotor.stopMotor();       
          eggsTurnerState = 2;
        }
        else{
          eggsTurnerStepperMotor.moveBackward(STEPPER_MOTOR_SPEED_DEFAULT);
          eggsTurnerState = 1;
        }        
        break;
      case 1: // WAIT_TO_REACH_LEFT_SIDE_INDUCTOR
        if(ccw_inductor_input.getInputState()){
          // home position is reached - full left
          eggsTurnerStepperMotor.stopMotor();
          eggsTurnerState = 2;
        }
        break;
      case 2: // WAIT_FOR_TURN_EGGS_COMMAND_STATE --> wait for RPI command
        if(stepperMotor_moveCCW_automatic_var){
          eggsTurnerStepperMotor.moveBackward(STEPPER_MOTOR_SPEED_DEFAULT);
          eggsTurnerState = 5;
        }
        if(stepperMotor_moveCW_automatic_var){
          eggsTurnerStepperMotor.moveForward(STEPPER_MOTOR_SPEED_DEFAULT);
          eggsTurnerState = 3;
        }
        if(stepperMotor_stop_automatic_var){
          eggsTurnerStepperMotor.stopMotor();
        }
        break;
      case 3: // WAIT_TO_REACH_RIGHT_SIDE_INDUCTOR
          if(cw_inductor_input.getInputState()){
            eggsTurnerStepperMotor.stopMotor();
            eggsTurnerState = 2;
          }
        break;
      case 5: // WAIT_TO_REACH_LEFT_SIDE_INDUCTOR
          if(ccw_inductor_input.getInputState()){
            eggsTurnerStepperMotor.stopMotor();
            eggsTurnerState = 2;
          }
        break;
      default:
        break;
    }
  }
  else{
    ;
    /*
    if(stepperAutomaticControl_trigger.catchFallingEdge()){ // catch della rimozione del controllo automatico: chiamo lo stop del motore.
      eggsTurnerStepperMotor.stopMotor();
      eggsTurnerState = 0; 
    }

    if(stepperMotor_moveForward_var){
      eggsTurnerStepperMotor.moveForward(STEPPER_MOTOR_SPEED_DEFAULT);
    }
    else if(stepperMotor_moveBackward_var){
      eggsTurnerStepperMotor.moveBackward(STEPPER_MOTOR_SPEED_DEFAULT);
    }
    else if(stepperMotor_stop_var){
      eggsTurnerStepperMotor.stopMotor();
    }
    */
  }  
  /* END STEPPER MOTOR CONTROL SECTION */

  
  
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

    strcpy(bufferChar, "<TMP04,");
    dtostrf( temperatures[3], 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(bufferChar, fbuffChar), ">");
    listofDataToSend_numberOfData++;

    float humidityDHT22 = temperatures[0];
    strcpy(bufferChar, "<HUM01,");
    dtostrf( humidityDHT22, 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(bufferChar, fbuffChar), ">");
    listofDataToSend_numberOfData++;

    strcpy(bufferChar, "<HTP01,"); // temperatura che viene letta dal sensore di umidità
    dtostrf( (temperatures[0] + temperatures[1] + temperatures[2]) / 3, 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(bufferChar, fbuffChar), ">");
    listofDataToSend_numberOfData++;

    /*
    // feedback about auxHeater state
    listofDataToSend[listofDataToSend_numberOfData] = switch1_state ? "<SWT01, 1>":"<SWT01, 0>"; // converting bool to string
    listofDataToSend_numberOfData++;
      */
    gotTemperatures = false;
  }
  
  // SENDING TO RPY
  
  if(listofDataToSend_numberOfData > 0){
    Serial.print('@'); // SYMBOL TO START BOARDS TRANSMISSION
    for(byte i = 0; i < listofDataToSend_numberOfData; i++){
      Serial.print(listofDataToSend[i]); 
    }
    Serial.println('#'); // SYMBOL TO END BOARDS TRANSMISSION
  }
  
  delay(50);

  /* reset command section */
  stepperMotor_moveCCW_automatic_var = false;
  stepperMotor_moveCW_automatic_var = false;
  stepperMotor_stop_automatic_var = false;
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

int readFromBoard(){ // returns the number of commands received
  receivingDataFromBoard = true;
  byte rcIndex = 0;
  char bufferChar[30];
  bool saving = false;
  bool enableReading = false;
  byte receivedCommandsIndex = 0;

  unsigned long  startReceiving; // mi ricordo appena entro in while loop

  while(receivingDataFromBoard){
    // qui è bloccante...assumo che prima o poi arduino pubblichi
    if(Serial.available() > 0){ // no while, perché potrei avere un attimo il buffer vuoto...senza aver ancora ricevuto il terminatore
      startReceiving = millis(); // ogni volta in cui leggo resetto il timer
      char rc = Serial.read(); 

      if(rc == '@'){ // // SYMBOL TO START TRANSMISSION (tutto quello che c'era prima era roba spuria che ho pulito)
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
        else if(rc == '#'){ // SYMBOL TO END BOARDS TRANSMISSION
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
    else{
      // timer che conta perché non riceviamo più e mi fa uscire??
      // se sono qui è perché sto aspettando, ma non ricevendo
      if((millis() - startReceiving) > 150){ // se passo in attesa più di 750ms, allora intervengo e mando fuori
        receivingDataFromBoard = false;
      }
    }
  }
  return receivedCommandsIndex;
}
