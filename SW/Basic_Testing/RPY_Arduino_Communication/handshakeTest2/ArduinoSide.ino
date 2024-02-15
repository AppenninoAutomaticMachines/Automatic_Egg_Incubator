char inData[10];
byte index = 0;
char EOP = ';';
String readString;
String incomingCommand;
String incomingNumber;


boolean ledBlink_enable = false;

int ledBlinkingConstant = 250;
int inputValue;
/*
GNRL001 ON command for LED_BUILTIN pin
GNRL002 OFF command for LED_BUILTIN pin

*/

// COMMANDS 
const byte NUMBER_OF_MAXIMUM_COMMANDS_PER_LOOP = 10;

String commandList[NUMBER_OF_MAXIMUM_COMMANDS_PER_LOOP]; //char message[# of elements][size of elements];
byte commandListIndex;

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

}

void readCommandsFromRpy(){
  Serial.print("wa#"); // notify RPY I'm waiting for commands

  while(!Serial.available()){ // waiting for RPY to communicate. FILTER OFF
    ;
  }

  boolean stillHaveCommandsToReceive = true;
  commandListIndex = 0; // keeps track of the number of commands sent
  while(stillHaveCommandsToReceive) { // read all commands
    if(Serial.available() > 0){ // single commmand reading: aa#
      incomingCommand = Serial.readStringUntil('#');
      Serial.read(); // to read # char

      if(incomingCommand == "zz"){ // end of commands
        Serial.print("rd#"); // read command done
        stillHaveCommandsToReceive = false; // out of the blocking while loop
      }
      else{
        if(commandListIndex < 10){
          Serial.print("rd#"); // read command done
          commandList[commandListIndex] = incomingCommand;
          commandListIndex++;
        }
        else{
          ; // PRINT ERROR! TOO MUCH COMMANDS
        }        
      }
      incomingCommand = "";
    }
  }
}


void loop() {
  readCommandsFromRpy();
  Serial.print("Ciao");

  for(int i = 0; i < commandListIndex; i++){
    Serial.print(commandList[i]);
  }
  while(true){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
  }
  
  /*
  if(incomingCommand == "GNRL001"){
    digitalWrite(LED_BUILTIN, HIGH);
  }
  */
  /*
  if(incomingCommand == "SET001"){ // dopo questo comando invio sempre il valore da settare
    incomingNumber = Serial.readStringUntil('#');
    Serial.read();
    Serial.println(incomingNumber);
    if (incomingNumber.length() > 0) {
      inputValue = incomingNumber.toInt();
      Serial.println(inputValue);
    }

    ledBlinkingConstant = inputValue;
    Serial.println(ledBlinkingConstant);
  }
  */

  
}

void ledBlink(byte pinNumber, unsigned long timingInterval){
  static unsigned long previousMillis_ledBlink;
  static bool ledState_ledBlink;

  unsigned long currentMillis_ledBlink = millis();

  if(currentMillis_ledBlink - previousMillis_ledBlink > timingInterval){
    previousMillis_ledBlink = currentMillis_ledBlink;

    if(ledState_ledBlink == LOW){
      ledState_ledBlink = HIGH;
      digitalWrite(pinNumber, ledState_ledBlink);
    }
    else{
      ledState_ledBlink = LOW;      
      digitalWrite(pinNumber, ledState_ledBlink);
    }
    Serial.print(ledState_ledBlink);
  }
}
