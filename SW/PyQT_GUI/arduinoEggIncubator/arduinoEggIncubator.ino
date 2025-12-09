/* LIBRARIES */
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <ProfiloLibrary.h>
#include <DHT.h>
#include "HX711.h"


/* General CONSTANTS */
#define SERIAL_SPEED 115200
#define NUMBER_OF_TEMPERATURES_SENSORS_ON_ONE_WIRE_BUS 4 //sensori di temperatura
#define DEFAULT_DEBOUNCE_TIME 25 //ms
#define ENABLE_HEATER true
#define ENABLE_HUMIDIFIER true
#define ENABLE_WATER_ELECTROVALVE true
#define TEMPERATURE_PRECISION 9 // DS18B20 digital termometer provides 9-bit to 12-bit Celsius temperature measurements
#define NUMBER_OF_LIGHTS 3
#if !defined(DEVICE_DISCONNECTED)
#define DEVICE_DISCONNECTED -127
#endif
#define DEVICE_ERROR 85

/* ARDUINO MEGA PWM from 0 - 13 + 0 and 1 tx and rx in case of serial communication is needed */
/* PIN ARDUINO */
#define DHT_PIN 3   //Pin a cui è connesso il sensore
#define ONE_WIRE_BUS 4
#define STEPPER_MOTOR_DIRECTION_PIN 5
#define STEPPER_MOTOR_STEP_PIN 6
#define FREE_PC817_PIN 7
#define CW_INDUCTOR_PIN 8 // induttore finecorsa DESTRO (vista posteriore)
#define CCW_INDUCTOR_PIN 9 // induttore finecorsa SINISTRO (vista posteriore)
#define WATER_ELECTROVALVE_PIN 10 // relay 4
#define HUMIDIFIER_PIN 11 // relay 3
#define HEATER_PIN 12 // relay 1
#define HEATER_PWM_PIN 13

#define ONE_WIRE_BUS_EXTERNAL_TEMPERATURE 22
#define FREE_OUTPUT_RELAY_PC817_2 23
#define PIN_RED_LIGHT 25
#define PIN_ORANGE_LIGHT 27
#define PIN_GREEN_LIGHT 29
#define PIN_BUZZER 31
#define LOADCELL_DOUT_PIN 33
#define LOADCELL_SCK_PIN 35


#define STEPPER_MOTOR_MS1_PIN 2
#define STEPPER_MOTOR_MS2_PIN 2
#define STEPPER_MOTOR_MS3_PIN 2


/* ALIVE su SERIALE */
/* se vedo che per più di 3.5s non ricevo segnale seriale, allora 1) allarme  2) inibisco i controlli degli attuatori.
  Per mantenere viva la comunicazione, ogni tot mando un comando di alive da python (perché altrimenti se non mando comandi manuali non ho motivo di fare niente)*/
bool alive_bit = false;
unsigned long last_serial_alive_time = 0;
unsigned long serial_alive_timeout_ms = 4000; // rpy manda un alive ogni 2secondi

/* se avvio il sistema senza però avviare il proramma di rpy, allora attenzione, non posso fare certe cose come muovere il motore.
  Finché leggo teperature e le mando al nulla ok, ma se si tratta di azionare attuatori devo aspettare l'alivenowledge dal rpy */
bool serial_communication_is_ok = false; 

/* TEMPERATURES SECTION */
// GENERAL
bool deviceOrderingActive = true; // di base prova ad ordinare con i sensori che conosciamo.
float marginFactor = 5; // fattore moltiplicativo per aspettare un po' più di delay.
unsigned long startGetTemperatures, endGetTemperatures;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress Thermometer[NUMBER_OF_TEMPERATURES_SENSORS_ON_ONE_WIRE_BUS];
byte numberOfDevices;
unsigned long conversionTime_DS18B20_sensors; //ms
unsigned long lastTempRequest;

/* Temperature sensors - addresses. LOWEST number = HIGHEST sensor, then follows the decreasing order */
// RICORDA LA VARIABILE ENABLE_DEVICE_ORDERING
char temperatureSensor_address0[] = "28FF640E7213DCBE";
char temperatureSensor_address1[] = "28FF640E7C2E42E0";
char temperatureSensor_address2[] = "28FF640E7F7492C3";
char temperatureSensor_address3[] = "28FF640E7F489F5E";

/* Temperature Diagnostic */
unsigned int deviceDisconnected[NUMBER_OF_TEMPERATURES_SENSORS_ON_ONE_WIRE_BUS];
unsigned int deviceError[NUMBER_OF_TEMPERATURES_SENSORS_ON_ONE_WIRE_BUS];

float temperatures[4]; 
bool gotTemperatures;

DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

// Define a char array to store the hexadecimal representation of the address
char addressCharArray[17]; // 16 characters for the address + 1 for null terminator

// EXTERNAL TEMPERATURE SENSOR
#define ENABLE_EXTERNAL_TEMPERATURE_READING true
OneWire oneWire_externalTemperatureSensor(ONE_WIRE_BUS_EXTERNAL_TEMPERATURE);
DallasTemperature externalTemperatureSensor(&oneWire_externalTemperatureSensor);
DeviceAddress externalTemperatureSensor_address;

unsigned int deviceDisconnected_externalTemperatureSensor;
unsigned int deviceError_externalTemperatureSensor;

float temperature_externalTemperatureSensor; 
/* END TEMPERATURES SECTION */



/* CCW_LS */
antiDebounceInput ccw_inductor_input(CCW_INDUCTOR_PIN, DEFAULT_DEBOUNCE_TIME);
trigger ccw_trigger;

/* CW_LS */
antiDebounceInput cw_inductor_input(CW_INDUCTOR_PIN, DEFAULT_DEBOUNCE_TIME);
trigger cw_trigger;


/* MOTORS SECTION */
#define STEPPER_MOTOR_SPEED_DEFAULT 10 //rpm  opportuno stare sotto i 30rpm, perché il tempo ciclo di arduino prende un po' troppo.

float stepper_motor_speed = STEPPER_MOTOR_SPEED_DEFAULT;

stepperMotor eggsTurnerStepperMotor(STEPPER_MOTOR_STEP_PIN, STEPPER_MOTOR_DIRECTION_PIN, STEPPER_MOTOR_MS1_PIN, STEPPER_MOTOR_MS2_PIN, STEPPER_MOTOR_MS3_PIN, 1.8);

bool move = false;
bool direction = false; 

bool stepperMotor_moveCCW_automatic_var = false;
bool stepperMotor_moveCW_automatic_var = false;
bool stepperMotor_stop_automatic_var = false;

bool stepperMotor_moveCCW_cmd = false;
trigger stepperMotor_moveCCW_cmd_trigger;

bool stepperMotor_moveCW_cmd = false;
trigger stepperMotor_moveCW_cmd_trigger;

bool stepperMotor_stop_cmd = false;
trigger stepperMotor_stop_cmd_trigger;

byte eggsTurnerState = 0;
bool stepperAutomaticControl_var = false;
/* END MOTORS SECTION */

/* DHT22 HUMIDITY SENSOR */
#define DHT_TYPE DHT22   //Tipo di sensore che stiamo utilizzando (DHT22)
DHT dht(DHT_PIN, DHT_TYPE); //Inizializza oggetto chiamato "dht", parametri: pin a cui è connesso il sensore, tipo di dht 11/22


int chk;
float humidity_fromDHT22;  //Variabile in cui verrà inserita la % di umidità
float temp_fromDHT22; //Variabile in cui verrà inserita la temperatura
/* END DHT22 HUMIDITY SENSOR */

/* HX711 WEIGHT CONTROL LOAD CELL */
HX711 scale;
int32_t calibration_offset = -79845; 
/* 
  The HX711’s raw readings are 24-bit signed values, which easily fit inside a 32-bit signed integer.
  The HX711 Arduino libraries typically use long, long int, or int32_t for offsets and raw data.
  int32_t guarantees a 32-bit integer on all platforms, which is safer than int (whose size varies).
*/
float calibration_scale = 420.0f; // to be extra explicit: In Arduino/C/C++, the f at the end of a number makes it a float literal instead of a double. .f tells the compiler the number is a float.
float waterWeight;
/*
  La lettura del sensore è decisamente molto lunga, in termini di tempo. Quindi se leggiamo il peso mentre il motore gira si vede molto l'interruzione del treno di step a causa della lettura.
  Sicuramente non posso fare una lettura del peso in simultanea con la temperatura. 
  E' necessario fare delle letture molto meno frequenti (magari con una periodicità multipla rispetto alle letture di temperatura).

  09/12/2025
  Faccio così: la lettura delle temperature avviene circa a 2 Hz.
  Metto qui una variabile che conta quante letture di temperature vengono fatte. Una volta ogni n letture di temperature, allora faccio anche la lettura del peso.
  Siccome devo comunque inviare in uscita un valore di peso (per la comunicazione seriale) mando l'ultimo più aggiornato.

  Per evitare che la lettura del peso influenzi la rotazione, prendo questa contromisura:
  1) SE IL MOTORE FUNZIONA --> saturazione alla lettura massima di 5.0 kg: così RPY se ne accorge e lui stesso comanda a FALSE l'elettrovalvola. Nello stesso momento, bypasso la chiamata
    alla lettura dell'adc (per risparmiare tempo) + metto in sleep mode.
  2) SE IL MOTORE SI FERMA --> riprendo la lettura normale a n Hz (multiplo intero della lettura delle temperature), così RPY si ribecca e decide se deve aprire l'elettrovalvola o no.

*/

int temperatureReadings_limitForWeightMeasurement = 5; /* variabile che dice ogni quante letture di temperatura dobbiamo fare una lettura del peso
                                                          ES. limite 5 letture  *  0.5 s/lettura = 2.5s intervallo di tempo fra due letture di peso */
int temperatureReadings_counter = 0;
bool enable_weightMeasurement = false;
float waterWeight_saturated = 5.0; // 5.0 kg LIMIT VALUE for this load cell                                                       
/* END HX711 WEIGHT CONTROL LOAD CELL */



/* MACHINE SINGALING DEVICE - SECTION */
const int lightPins[NUMBER_OF_LIGHTS] = {PIN_RED_LIGHT, PIN_ORANGE_LIGHT, PIN_GREEN_LIGHT}; // Light pins
const int buzzerPin = PIN_BUZZER; // Buzzer pin

unsigned long lastUpdate = 0;
const unsigned long runInterval = 100; // Run every 100ms

enum State { OFF, ON, FLASH_FAST, FLASH_SLOW, BEEP_FAST, BEEP_SLOW };

struct Device {
    State state;
    unsigned long lastToggle;
    bool currentState;
};

Device lights[NUMBER_OF_LIGHTS];
Device buzzer;
/* END MACHINE SINGALING DEVICE - SECTION */

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
  pinMode(HEATER_PWM_PIN, OUTPUT);
  digitalWrite(HEATER_PWM_PIN, LOW);

  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);

  pinMode(HUMIDIFIER_PIN, OUTPUT);
  digitalWrite(HUMIDIFIER_PIN, LOW);

  pinMode(WATER_ELECTROVALVE_PIN, OUTPUT);
  digitalWrite(WATER_ELECTROVALVE_PIN, LOW);

  pinMode(FREE_OUTPUT_RELAY_PC817_2, OUTPUT);
  digitalWrite(FREE_OUTPUT_RELAY_PC817_2, LOW);

  pinMode(STEPPER_MOTOR_STEP_PIN, OUTPUT);
  digitalWrite(STEPPER_MOTOR_STEP_PIN, LOW);

  pinMode(STEPPER_MOTOR_DIRECTION_PIN, OUTPUT);
  digitalWrite(STEPPER_MOTOR_DIRECTION_PIN, LOW);  

  pinMode(STEPPER_MOTOR_MS1_PIN, OUTPUT);
  digitalWrite(STEPPER_MOTOR_MS1_PIN, LOW);

  pinMode(STEPPER_MOTOR_MS2_PIN, OUTPUT);
  digitalWrite(STEPPER_MOTOR_MS2_PIN, LOW);

  pinMode(STEPPER_MOTOR_MS3_PIN, OUTPUT);
  digitalWrite(STEPPER_MOTOR_MS3_PIN, LOW);

  pinMode(CCW_INDUCTOR_PIN, INPUT);
  pinMode(CW_INDUCTOR_PIN, INPUT);

  /* MACHINE SINGALING DEVICE - SECTION */
  for (int i = 0; i < NUMBER_OF_LIGHTS; i++) {
        pinMode(lightPins[i], OUTPUT);
        digitalWrite(lightPins[i], LOW);
        lights[i] = {OFF, 0, LOW};
    }
    pinMode(buzzerPin, OUTPUT);
    digitalWrite(buzzerPin, LOW);
    buzzer = {OFF, 0, LOW};

  Serial.begin(SERIAL_SPEED);
  //Serial.println("Starting");

  /* HX 711 initializing water scale */
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_offset(calibration_offset);
  scale.set_scale(calibration_scale);
  //scale.tare();

  sensors.begin();

  // locate devices on the bus
  //Serial.print("Locating devices...");
  //Serial.print("Found ");

  numberOfDevices = sensors.getDeviceCount();


  sensors.setWaitForConversion(false); // quando richiedi le temperature requestTemperatures() la libreria NON aspetta il delay adeguato, quidni devi aspettarlo tu.
  sensors.requestTemperatures(); // send command to all the sensors for temperature conversion.
   
  if (ENABLE_EXTERNAL_TEMPERATURE_READING){
    externalTemperatureSensor.begin();
    externalTemperatureSensor.setWaitForConversion(false); // quando richiedi le temperature requestTemperatures() la libreria NON aspetta il delay adeguato, quidni devi aspettarlo tu.
    externalTemperatureSensor.requestTemperatures(); // send command to all the sensor for temperature conversion.
  }
    

  lastTempRequest = millis();
  
  conversionTime_DS18B20_sensors = 750 / (1 << (12 - TEMPERATURE_PRECISION));  // res in {9,10,11,12}

  for(uint8_t index = 0; index < numberOfDevices; index++){
    if(sensors.getAddress(tempDeviceAddress, index)){ // fetch dell'indirizzo
      addressToCharArray(tempDeviceAddress, addressCharArray); // indirizzo convertito

      // ora ordino il vettore dei sensori.
      if(strcmp(addressCharArray, temperatureSensor_address0) == 0){ // returns 0 when the two strings are identical
        sensors.getAddress(Thermometer[0], index);
      }
      else if(strcmp(addressCharArray, temperatureSensor_address1) == 0){ 
        sensors.getAddress(Thermometer[1], index);
      }
      else if(strcmp(addressCharArray, temperatureSensor_address2) == 0){ 
        sensors.getAddress(Thermometer[2], index);
      }
      else if(strcmp(addressCharArray, temperatureSensor_address3) == 0){ 
        sensors.getAddress(Thermometer[3], index);
      }
      else{
        // evidentemente indirizzo non presente, allora non procedo con il device ordering
        deviceOrderingActive = false;
      }       

      // initializing arrays - l'azzeramento posso farlo senza posizioni, non importa, tanto è tutto a 0.
      deviceDisconnected[index] = 0;
      deviceError[index] = 0;

      delay(5);
    }
  }

  if(!deviceOrderingActive){
    // se si è disattivato perché è cambiato un sensore, allora devo riciclarli tutti e metterli nel vettore a caso.
    for(uint8_t index = 0; index < numberOfDevices; index++){
      sensors.getAddress(Thermometer[index], index);
      deviceDisconnected[index] = 0;
      deviceError[index] = 0;
    }
  }

  if (ENABLE_EXTERNAL_TEMPERATURE_READING){
    if(externalTemperatureSensor.getAddress(tempDeviceAddress, 0)){
      addressToCharArray(tempDeviceAddress, addressCharArray); // indirizzo convertito
      externalTemperatureSensor.getAddress(externalTemperatureSensor_address, 0);
      deviceDisconnected_externalTemperatureSensor = 0;
      deviceError_externalTemperatureSensor = 0;
    }
  }
    

  last_serial_alive_time = millis();
  dht.begin();
}

void loop() {    
  // RECEIVING FROM RPI
  if(Serial.available() > 0){ 
    int numberOfCommandsFromBoard = readFromBoard(); // from ESP8266. It has @ as terminator character
    last_serial_alive_time = millis();
    String pendingACK; // mandiamo un comando per volta, quindi ci sarà un solo comando a ciclo for.
    // Guardiamo che comandi ci sono arrivati
    for (byte j = 0; j < numberOfCommandsFromBoard; j++) {
      String tempReceivedCommand = receivedCommands[j];
      //Serial.println(tempReceivedCommand);
      String tag, value, uid;
      if (splitCommand(tempReceivedCommand, tag, value, uid)) {
        if (tag == "ALIVE") {
          last_serial_alive_time = millis();
          if (value == "True") {
            alive_bit = true;
            serial_communication_is_ok = true;
          } else if (value == "False") {
            alive_bit = false;
            serial_communication_is_ok = true;
          }
        }

        if (tag == "HTR01") {
          if (value == "True") {
            digitalWrite(HEATER_PIN, HIGH);
          } else if (value == "False") {
            digitalWrite(HEATER_PIN, LOW);
          }
          // Sposta la generazione ACK qui, fuori dal parsing
          if(uid.length() > 0){
              pendingACK = "<" + tag + ", " + value + ", " + uid + ">";
          }
        }
          

        if (tag == "HUMER01") {
          if (value == "True") {
            digitalWrite(HUMIDIFIER_PIN, HIGH);
          } else if (value == "False") {
            digitalWrite(HUMIDIFIER_PIN, LOW);
          }
          // Sposta la generazione ACK qui, fuori dal parsing
          if(uid.length() > 0){
              pendingACK = "<" + tag + ", " + value + ", " + uid + ">";
          }
        }

        if (tag == "STPR01") {
          if (value == "MCCW") {
            stepperMotor_moveCCW_automatic_var = true;
            stepperMotor_moveCCW_cmd = true;
          } else if (value == "MCW") {
            stepperMotor_moveCW_automatic_var = true;
            stepperMotor_moveCW_cmd = true;
          } else if (value == "STOP") {
            stepperMotor_stop_automatic_var = true;
            stepperMotor_stop_cmd = true;
          }
          // Sposta la generazione ACK qui, fuori dal parsing
          if(uid.length() > 0){
              pendingACK = "<" + tag + ", " + value + ", " + uid + ">";
          }
        }

        if (tag == "ELV01") {
          if (value == "True") {
            digitalWrite(WATER_ELECTROVALVE_PIN, HIGH);
          } else if (value == "False") {
            digitalWrite(WATER_ELECTROVALVE_PIN, LOW);
          }
          // Sposta la generazione ACK qui, fuori dal parsing
          if(uid.length() > 0){
              pendingACK = "<" + tag + ", " + value + ", " + uid + ">";
          }
        }

        if (tag == "PWM01") {
          int pwm_value = value.toInt();
          analogWrite(HEATER_PWM_PIN, pwm_value);
          // Sposta la generazione ACK qui, fuori dal parsing
          if(uid.length() > 0){
              pendingACK = "<" + tag + ", " + value + ", " + uid + ">";
          }
        }
      }
      else{
        // comando malformato, non lo considero valido + eventuale log
        continue;
      }
    }
    // fuori dal ciclo for, mando gli ack
    if(pendingACK.length() > 0){
        Serial.print('@');
        Serial.print(pendingACK);
        Serial.println('#');
        pendingACK = "";
        delay(1);
    }
  }
  else{
    if(millis() - last_serial_alive_time > serial_alive_timeout_ms){
      // se per più di 1secondo non ricevo roba dalla seriale (che significa che non sta arrivando più nemmeno alive), allora inibisci le cose da inibire
      serial_communication_is_ok = false;
      eggsTurnerStepperMotor.stopMotor();
      digitalWrite(HEATER_PIN, LOW);
      digitalWrite(HUMIDIFIER_PIN, LOW);
      digitalWrite(WATER_ELECTROVALVE_PIN, LOW);
      analogWrite(HEATER_PWM_PIN, 0);
    }
  }

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
    }

    gotTemperatures = true;  
    sensors.requestTemperatures();

    if (ENABLE_EXTERNAL_TEMPERATURE_READING){
      temperature_externalTemperatureSensor = externalTemperatureSensor.getTempC(externalTemperatureSensor_address);
      if(temperature_externalTemperatureSensor <= DEVICE_DISCONNECTED){
        deviceDisconnected_externalTemperatureSensor ++;
      }
      if(temperature_externalTemperatureSensor >= DEVICE_ERROR){
        deviceError_externalTemperatureSensor ++;
      }       
      externalTemperatureSensor.requestTemperatures();
    }      

    lastTempRequest = millis();
    endGetTemperatures = millis();

    temperatureReadings_counter ++;
  }   
  /* END TEMPERATURES SECTION */

  /* HX 711 WATER WEIGHT MEASUREMENT - LOAD CELL */
  if (temperatureReadings_counter >= temperatureReadings_limitForWeightMeasurement){
    // ogni n letture di temperatura abilitiamo la lettura del peso
    temperatureReadings_counter = 0; // reset counter

    // prima di leggere, fare un check se il motore sta andando o no.
    enable_weightMeasurement = !(eggsTurnerStepperMotor.isMovingBackward() || eggsTurnerStepperMotor.isMovingForward()); // The line sets enable_weightMeasurement to true only when the stepper motor is NOT moving.
    if (enable_weightMeasurement){
      // motore non va, allora possiamo leggere il dato dall'ADC (MOLTO TIME CONSUMING!)
      scale.power_up();	
      waterWeight = scale.get_units(5);
      waterWeight = round(waterWeight * 10.0) / 10.0; // arrotondamento ad una cifra decimale
      scale.power_down();
    }
    else{
      // il motore sta funzionando: saturazione a 5.0 kg + esclusione del codice di lettura della load cell.
      waterWeight = waterWeight_saturated;
    }
  }    

  /* DHT22 HUMIDITY SENSOR */
  /* DHT22 sensor */    
  humidity_fromDHT22 = dht.readHumidity();
  temp_fromDHT22 = dht.readTemperature();   
  /* END DHT22 HUMIDITY SENSOR */

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

  
  // direzione oraria = forward = clock wise rotation direction (vista posteriore incubatrice)
  switch(eggsTurnerState){
    case 0:
      // stato in cui dobbiamo aspettare che il pogramma lato python sia partito --> guardiamo la comunicazione seriale
      if(serial_communication_is_ok){
        eggsTurnerState = 1;
      }
      break;
    case 1: // waiting: prima di iniziare la procedura, voglio aspettare un po' che le comunicazioni siano partite e tutto...
      if(millis() >= 5000){
        // apena passano 5secondi, fai muovere il motore per azzerare
        eggsTurnerState = 9;        
      }
      
      break;
    case 9: // ZEROING
      // lascio stato aperto per eventuale abilitazione della procedura di azzeramento da parte dell'operatore.
      if(ccw_inductor_input.getInputState()){ // se leggo già induttore, inutile comandare un bw. Sono già azzerato.  Devo però notificarlo a RPI!
        // home position is reached - full left
        listofDataToSend[listofDataToSend_numberOfData] = "<IND_CCW, 1>"; 
        listofDataToSend_numberOfData++;

        eggsTurnerStepperMotor.stopMotor();       
        eggsTurnerState = 20;
      }
      else if(cw_inductor_input.getInputState()){ // se leggo già induttore. Sono già azzerato.  Devo però notificarlo a RPI!
        // home position is reached - full right
        listofDataToSend[listofDataToSend_numberOfData] = "<IND_CW, 1>";
        listofDataToSend_numberOfData++;
        
        eggsTurnerStepperMotor.stopMotor();       
        eggsTurnerState = 20;
      }
      else{
        eggsTurnerStepperMotor.moveBackward(STEPPER_MOTOR_SPEED_DEFAULT);
        eggsTurnerState = 10;
      }        
      break;
    case 10: // WAIT_TO_REACH_LEFT_SIDE_INDUCTOR
      if(ccw_inductor_input.getInputState()){
        // home position is reached - full left
        eggsTurnerStepperMotor.stopMotor();
        eggsTurnerState = 20;
      }
      break;
    case 20: 
      if(stepperAutomaticControl_var){
        /* FULLY AUTOMATIC MODE in arduino side*/
        // WAIT_FOR_TURN_EGGS_COMMAND_STATE --> wait for RPI command
        if(stepperMotor_moveCCW_automatic_var){
          eggsTurnerStepperMotor.moveBackward(STEPPER_MOTOR_SPEED_DEFAULT);
          eggsTurnerState = 50;
        }
        if(stepperMotor_moveCW_automatic_var){
          eggsTurnerStepperMotor.moveForward(STEPPER_MOTOR_SPEED_DEFAULT);
          eggsTurnerState = 30;
        }
        if(stepperMotor_stop_automatic_var){
          eggsTurnerStepperMotor.stopMotor();
        }
      }
      else{
        /* MANUAL MODE - tutti i comandi vengono da RPI */
        eggsTurnerState = 100;
      }
      break;
    case 30: // WAIT_TO_REACH_RIGHT_SIDE_INDUCTOR
        if(cw_inductor_input.getInputState()){
          eggsTurnerStepperMotor.stopMotor();
          eggsTurnerState = 20;
        }
      break;
    case 50: // WAIT_TO_REACH_LEFT_SIDE_INDUCTOR
        if(ccw_inductor_input.getInputState()){
          eggsTurnerStepperMotor.stopMotor();
          eggsTurnerState = 20;
        }
      break;
    case 100:
      stepperMotor_moveCCW_cmd_trigger.periodicRun(stepperMotor_moveCCW_cmd);
      stepperMotor_moveCW_cmd_trigger.periodicRun(stepperMotor_moveCW_cmd);
      stepperMotor_stop_cmd_trigger.periodicRun(stepperMotor_stop_cmd);

      if(stepperMotor_moveCCW_cmd_trigger.catchRisingEdge()){
        if(ccw_inductor_input.getInputState()){
          // se leggo già, allora non fare nulla (meccanicamente impossibile..io a mano che ciapino). Devo solo notificarlo a RPY
          // home position is reached - full CCW
          listofDataToSend[listofDataToSend_numberOfData] = "<IND_CCW, 1>"; 
          listofDataToSend_numberOfData++;
        }
        else{
          /* MECHANICAL SAFETY - abilito il movimento solo se NON sto già leggendo */
          eggsTurnerStepperMotor.moveBackward(STEPPER_MOTOR_SPEED_DEFAULT);
        }
      }
      else if(stepperMotor_moveCW_cmd_trigger.catchRisingEdge()){
        if(cw_inductor_input.getInputState()){
          // se leggo già, allora non fare nulla (meccanicamente impossibile..io a mano che ciapino). Devo solo notificarlo a RPY
          // home position is reached - full CW
          listofDataToSend[listofDataToSend_numberOfData] = "<IND_CW, 1>"; 
          listofDataToSend_numberOfData++;
        }
        else{
          /* MECHANICAL SAFETY - abilito il movimento solo se NON sto già leggendo */
          eggsTurnerStepperMotor.moveForward(STEPPER_MOTOR_SPEED_DEFAULT);
        }
      }
      else if(stepperMotor_stop_cmd_trigger.catchRisingEdge()){
        eggsTurnerStepperMotor.stopMotor();
      }

      /* MECHANICAL SAFETY */
      
      if(ccw_inductor_input.getInputState() and eggsTurnerStepperMotor.isMovingBackward()){
        eggsTurnerStepperMotor.stopMotor();
      }
      if(cw_inductor_input.getInputState() and eggsTurnerStepperMotor.isMovingForward()){
        eggsTurnerStepperMotor.stopMotor();
      }
      
      break;
    default:
      break;
  }
  /* END STEPPER MOTOR CONTROL SECTION */

  /* MACHINE SINGALING DEVICE - SECTION */
  unsigned long currentTime = millis();
    if (currentTime - lastUpdate >= runInterval) {
        for (int i = 0; i < NUMBER_OF_LIGHTS; i++) updateDevice(lights[i], lightPins[i], 200, 500);
        updateDevice(buzzer, buzzerPin, 100, 300);
        lastUpdate = currentTime;
    }
  /* END MACHINE SINGALING DEVICE - SECTION */
  
  
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

    strcpy(bufferChar, "<HUM01,");
    dtostrf(humidity_fromDHT22, 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(bufferChar, fbuffChar), ">");
    listofDataToSend_numberOfData++;

    strcpy(bufferChar, "<HTP01,"); // temperatura che viene letta dal sensore di umidità
    dtostrf(temp_fromDHT22, 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(bufferChar, fbuffChar), ">");
    listofDataToSend_numberOfData++;

    strcpy(bufferChar, "<EXTT,");
    dtostrf(temperature_externalTemperatureSensor, 1, 1, fbuffChar); 
    listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(bufferChar, fbuffChar), ">");
    listofDataToSend_numberOfData++;

    strcpy(bufferChar, "<WGT01,");
    dtostrf(waterWeight, 1, 1, fbuffChar); 
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
  listofDataToSend_numberOfData = 0;

  delay(1);

  /* reset command section */
  stepperMotor_moveCCW_automatic_var = false;
  stepperMotor_moveCW_automatic_var = false;
  stepperMotor_stop_automatic_var = false;

  stepperMotor_moveCCW_cmd = false;
  stepperMotor_moveCW_cmd = false;
  stepperMotor_stop_cmd = false;

  /* DEBUG */
  //digitalWrite(RED_LED, !serial_communication_is_ok);
  //digitalWrite(ALIVE, alive_bit);

  if(!ENABLE_HEATER){
    digitalWrite(HEATER_PIN, LOW);
  }

  if(!ENABLE_HUMIDIFIER){
    digitalWrite(HUMIDIFIER_PIN, LOW);
  }

  if(!ENABLE_WATER_ELECTROVALVE){
    digitalWrite(WATER_ELECTROVALVE_PIN, LOW);
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

void updateDevice(Device &device, int pin, unsigned long fastInterval, unsigned long slowInterval) {
    unsigned long currentTime = millis();
    unsigned long interval = (device.state == FLASH_FAST || device.state == BEEP_FAST) ? fastInterval : slowInterval;
    
    switch (device.state) {
        case OFF:
            digitalWrite(pin, LOW);
            device.currentState = LOW;
            break;
        case ON:
            digitalWrite(pin, HIGH);
            device.currentState = HIGH;
            break;
        case FLASH_FAST:
        case FLASH_SLOW:
        case BEEP_FAST:
        case BEEP_SLOW:
            if (currentTime - device.lastToggle >= interval) {
                device.currentState = !device.currentState;
                digitalWrite(pin, device.currentState);
                device.lastToggle = currentTime;
            }
            break;
    }
}

bool splitCommand(String input, String &tag, String &value, String &uid) {
  // funzione di parsing
  input.trim();
  
  int firstComma = input.indexOf(',');
  int secondComma = input.indexOf(',', firstComma + 1);

  if (firstComma < 0) return false;

  tag = input.substring(0, firstComma);
  
  if (secondComma < 0) {
    value = input.substring(firstComma + 1);
    uid = "";
  } else {
    value = input.substring(firstComma + 1, secondComma);
    uid = input.substring(secondComma + 1);
  }

  tag.trim();
  value.trim();
  uid.trim();

  return true;
}
