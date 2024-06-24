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
#define WATCHDOG_ENABLE false
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
#define TEMPERATURE_CYCLICITY_MS 1000

DeviceAddress Thermometer[MAX_SENSORS];
byte numberOfDevices;
byte Limit;
unsigned long conversionTime_DS18B20_sensors; // in millis
unsigned long lastTempRequest;

#if !defined(DEVICE_DISCONNECTED)
#define DEVICE_DISCONNECTED -127
#endif

temperatureController temperatureController;

bool automaticControl_var = true; //di default partiamo con controllo automatico a true

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

bool mainHeaterSelected = true;
float temperature_heater_selection_lower_treshold, temperature_heater_selection_upper_treshold;

trigger mainHeaterSeleted_trigger; // rising/falling edge di quando dobbiamo usare il riscaldatore principale
trigger temperatureController_outputState_trigger; // rising/falling edge di quando dobbiamo scaldare o no. Ogni volta che cambia, mettiamo un delay per essere sicuri di non fare accendi/spegni con mal-letture dei sensori
/* END TEMPERATURES SECTION */

/* DHT22 HUMIDITY SENSOR */
#define DHTTYPE DHT22   //Tipo di sensore che stiamo utilizzando (DHT22)
DHT dht(DHTPIN, DHTTYPE); //Inizializza oggetto chiamato "dht", parametri: pin a cui è connesso il sensore, tipo di dht 11/22


int chk;
float humidity_fromDHT22;  //Variabile in cui verrà inserita la % di umidità
float temp_fromDHT22; //Variabile in cui verrà inserita la temperatura

float humiditySensor_temperature; // variabile buffer in cui, durante la lettura dei sensori DS18B20, metto quella del bulbo umido
float temperatureMeanValue; // temperatura media dell'incubatrice calcolata dall'oggetto temperatureController
float wetTermometer_fromDS18B20; // temperatura = humiditySensor_temperature che uso in sezione umidità del programma
/* END DHT22 HUMIDITY SENSOR */


/* MOTORS SECTION */
#define STEPPER_MOTOR_SPEED_DEFAULT 5 //rpm  opportuno stare sotto i 30rpm, perché il tempo ciclo di arduino prende un po' troppo.

float stepper_motor_speed = STEPPER_MOTOR_SPEED_DEFAULT;

stepperMotor eggsTurnerStepperMotor(STEPPPER_MOTOR_STEP_PIN, STEPPPER_MOTOR_DIRECTION_PIN, STEPPPER_MOTOR_MS1_PIN, STEPPPER_MOTOR_MS2_PIN, STEPPPER_MOTOR_MS3_PIN, 1.8);

bool move = false;
bool direction = false; 

bool stepperAutomaticControl_var = true; // parto di default a true con il girauova

antiDebounceInput leftInductor_input(LEFT_INDUCTOR_PIN, DEFAULT_DEBOUNCE_TIME);
antiDebounceInput rightInductor_input(RIGHT_INDUCTOR_PIN, DEFAULT_DEBOUNCE_TIME);

// variabile per manual control
bool stepperMotor_moveForward_var = false;
bool stepperMotor_moveBackward_var = false;
bool stepperMotor_stop_var = false;

byte eggsTurnerState = 0;
bool turnEggs_cmd = false;

trigger stepperAutomaticControl_trigger;
trigger automaticControl_trigger;

unsigned int numberOfEggTurns_counter = 0;
/* END MOTORS SECTION */



#define SECONDS_CONFIGURATION true
int seconds_trigger_interval = 3600; //  TESTING: giriamo ogni 3 minuti
int secondsToGO; // minuti mancanti alla prossima girata
int secondsGone; // minuti passati dalla precedente girata

/* END RTC SECTION */

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

char bufferCharArray[35]; // buffer lo metto qui
// handling float numbers
char bufferChar[35];
char fbuffChar[15];

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

  pinMode(CYCLE_TOGGLE_PIN, OUTPUT);
  pinMode(CYCLE_TOGGLE_ANALOG0_PIN, OUTPUT);


  pinMode(MAIN_HEATER_PIN, OUTPUT);
  pinMode(AUX_HEATER_PIN, OUTPUT);

  pinMode(UPPER_FAN_PIN, OUTPUT);
  pinMode(LOWER_FAN_PIN, OUTPUT);

  // resetting an putting to low level FAN commands:
  delay(250);
  digitalWrite(UPPER_FAN_PIN, LOW); // logica negata, in connessione hw: normalmente chiuso per non dover eccitare costanemente il relè
  delay(250);
  digitalWrite(LOWER_FAN_PIN, LOW); // logica negata, in connessione hw: normalmente chiuso per non dover eccitare costanemente il relè

  pinMode(LEFT_INDUCTOR_PIN, INPUT);
  pinMode(RIGHT_INDUCTOR_PIN, INPUT);

  Serial.begin(SERIAL_SPEED);

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
    }
  } 
  /* END TEMPERATURES SECTION */

  dht.begin();

  delay(2500);
  serialFlush();

  digitalWrite(LED_BUILTIN, HIGH); // all'avvio accendo il LED_BUILTIN. Durante il funzionamento lo resetto. Se ritorno e lo vedo acceso, significa che ha agito il watchdog.
}

void loop() {    
  /* TEMPERATURES SECTION */
  if(millis() - lastTempRequest >= TEMPERATURE_CYCLICITY_MS){
    controlTemperatureIndex = 0;
    for(byte index = 0; index < Limit; index++){
      // Call the function to convert the device address to a char array
      addressToCharArray(Thermometer[index], addressCharArray);
      if(strcmp(addressCharArray, comparisonAddress) == 0){
        // calcolo dell'umidità
        humiditySensor_temperature = sensors.getTempC(Thermometer[index]);
        if(humiditySensor_temperature < 0.0){
          humiditySensor_temperature = 1.5;
        }
      }
      else{
        // gli altri li possiamo streammare verso html
        temperatures[controlTemperatureIndex] = sensors.getTempC(Thermometer[index]); // memorizza tutte le temperature che ci pensa l'oggetto temperatureController a gestirle
        if(temperatures[controlTemperatureIndex] < 0.0){
          temperatures[controlTemperatureIndex] = 1.5;
        }
        controlTemperatureIndex += 1;
      }     
      delay(5);
    }
    gotTemperatures = true;
  
    sensors.requestTemperatures();
    delay(10);
    lastTempRequest = millis();
  } 
  /* END TEMPERATURES SECTION */



  /* SERIAL COMMUNICATION FROM ARDUINO TO RASPBERRY: SEND BACK SENSOR VALUES AND OTHER INFOS */
  // riempo i dati per la trasmissione
  listofDataToSend_numberOfData = 0; // ad ogni giro lo azzero  

  if(gotTemperatures){ // questo if riempe la trasmissione con i dati delle temperature. (ma potrei mandarne in modo asincrono anche altri...)
    gotTemperatures = false;    

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
  }

  /* 
    Lascio spazio per riempire il buffer di trasmissione con cose che non siano delle temperature...
    es. ho finito il turn delle uova...oppure è stata aperta la porta dell'incubatrice 
  */

  if(listofDataToSend_numberOfData > 0){
    Serial.print("@"); // SYMBOL TO START BOARDS TRANSMISSION
    for(byte i = 0; i < listofDataToSend_numberOfData; i++){
      Serial.print(listofDataToSend[i]); 

    }
    Serial.print("#"); // SYMBOL TO END BOARDS TRANSMISSION
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

void serialFlush(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}
