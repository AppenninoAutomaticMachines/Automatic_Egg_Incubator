// Facciamo che raspberry manda comando e aspetta il DONE
// Arduino fa steram dei dati ad ogni giro. Ci penserà poi RPI se leggerli o no...Arduino li butta fuori e fine.
// Arduino stream in out; Raspberry manda comandi a polling

char inData[10];
byte index = 0;
char EOP = ';';
String readString;
String incomingCommand;
String incomingNumber;

/*
String sendCommandList = "CMDX001# CMDX002# SRVMT003# TMPXX005# 
                          GNRL001# GNRL002# GNRL003# SET001# 650# 1000# 100#";
*/

boolean ledBlink_enable = false;

int ledBlinkingConstant = 250;
int inputValue;
/*
GNRL001 ON command for LED_BUILTIN pin
GNRL002 OFF command for LED_BUILTIN pin

*/
void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {

  if(Serial.available() > 0) {
    incomingCommand = Serial.readStringUntil('#');
    Serial.read(); // legge un solo carattere dal buffer: pulisce il #
    // Posso inviare comandi multipli. Faccio tutto il giro (del loop) e il buffer rimane pieno.
    // Pian piano lo svuoto. Oppure mi metto in while e mi blocco a leggere tutto
    if (incomingCommand.length() > 0) {
      Serial.println(incomingCommand);  
      Serial.print("Point1");    
      //Serial.flush();
    }
  }
  
  if(incomingCommand == "GNRL001"){
    digitalWrite(LED_BUILTIN, HIGH);
  }

  if(incomingCommand == "GNRL002"){
    digitalWrite(LED_BUILTIN, LOW);
  }

  if(incomingCommand == "GNRL003"){
    ledBlink_enable = true;
  }

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

  if(ledBlink_enable){
    ledBlink(LED_BUILTIN, ledBlinkingConstant);
  }
  incomingCommand = "";
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
  }
}
