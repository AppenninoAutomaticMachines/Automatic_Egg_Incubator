// https://www.martyncurrey.com/esp8266-and-the-arduino-ide-part-8-auto-update-webpage/
// https://www.instructables.com/UNO-R3-WIFI-ESP8266-CH340G-Arduino-and-WIFI-a-Vers/
/*
 ESP8266 modalità programmazione: Generic ESP8266 Module a 1 i bit 5 6 7 Mode 2 (SW5, SW6, SW7 are On)
 ESP8266 modalità funzionamento: Generic ESP8266 Module  a 1 i bit 5 6 (SW5 off, SW6, SW7 are On)
*/

/*
Soft WDT reset - NodeMCU
https://forum.arduino.cc/t/soft-wdt-reset-nodemcu/425567/2
The ESP8266 is a little different than the standard Arduino boards in that it has the watchdog (WDT) turned on by default. If the watchdog timer isn't periodically reset then it will automatically reset your ESP8266. The watchdog is reset every time one of the following occurs:

Return from loop() (i.e., reach the end of the function)
You call delay()
You call yield()
*/

String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

String html_1 = R"=====(
<!--https://randomnerdtutorials.com/esp8266-web-server/-->
<!DOCTYPE html>
<html>
 <head>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'/>
  <meta charset='utf-8'>
  <style>
    body {
      font-family: Verdana, sans-serif;
      font-size:100%;
    }
    #temperatures_section {display: table; margin: auto;  padding: 10px 10px 10px 10px; } 
    #content_temperatures_section { border: 5px solid blue; border-radius: 15px; padding: 10px 0px 10px 0px;}

    #commands_section {display: table; margin: auto;  padding: 10px 10px 10px 10px; } 
    #content_commands_section { border: 5px solid green; border-radius: 15px; padding: 10px 0px 10px 0px;}

    h2 {text-align:center; margin: 10px 0px 10px 0px;} 
    p { text-align:center; margin: 5px 0px 10px 0px; font-size: 120%;}
    #time_P { margin: 10px 0px 15px 0px;}

    .button {
      border: none;
      padding: 16px 40px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 30px;
      margin: 4px 2px;
      transition-duration: 0.4s;
      cursor: pointer;
    }

   .buttonOn {
      background-color: #81B57B;
      color: black;
      border: 4px solid #13BD00; /* green */
    }

   .buttonOff {
      background-color: #D97E7E;
      color: black;
      border: 4px solid #C20000; /* red */
    }

    
  </style>
 
  <script> 
    function updateTime() 
    {  
       var d = new Date();
       var t = "";
       t = d.toLocaleTimeString();
       document.getElementById('P_time').innerHTML = t;
    }
 
    function updateTemp() 
    {  
       ajaxLoad('getTemp'); 
    }
 
    var ajaxRequest = null;
    if (window.XMLHttpRequest)  { ajaxRequest =new XMLHttpRequest(); }
    else                        { ajaxRequest =new ActiveXObject("Microsoft.XMLHTTP"); }
 
    function ajaxLoad(ajaxURL)
    {
      if(!ajaxRequest){ alert('AJAX is not supported.'); return; }
 
      ajaxRequest.open('GET',ajaxURL,true);
      ajaxRequest.onreadystatechange = function()
      {
        if(ajaxRequest.readyState == 4 && ajaxRequest.status==200)
        {
          var ajaxResult = ajaxRequest.responseText;
          var tmpArray = ajaxResult.split("|");
          document.getElementById('temp_sensor1').innerHTML = tmpArray[0];
          document.getElementById('temp_sensor2').innerHTML = tmpArray[1];
          document.getElementById('temp_sensor3').innerHTML = tmpArray[2];
        }
      }
      ajaxRequest.send();
    }
 
    var myVar1 = setInterval(updateTemp, 750);  
    var myVar2 = setInterval(updateTime, 750);  

    document.addEventListener("DOMContentLoaded", function (event) {
        var scrollpos = sessionStorage.getItem('scrollpos');
        if (scrollpos) {
            window.scrollTo(0, scrollpos);
            sessionStorage.removeItem('scrollpos');
        }
    });

    window.addEventListener("beforeunload", function (e) {
        sessionStorage.setItem('scrollpos', window.scrollY);
    });
 
  </script>
 
 
  <title>Interfaccia Incubatrice Automatica</title>
 </head>
 
 <body>
   <p id='P_time'>Time</p>
   <p><a href="/stepperMotorControlPage">CONTROLLO MOTORE STEPPER</a></p>
   <div id='temperatures_section'>     
     <div id='content_temperatures_section'> 
       <h2>Temperatura: sensore alto</h2>
       <p> <span id='temp_sensor1'>--.-</span> &deg;C </p>
       <h2>Temperatura: sensore intermedio</h2>
       <p> <span id='temp_sensor2'>--.-</span> &deg;C </p>
       <h2>Temperatura: sensore basso</h2>
       <p> <span id='temp_sensor3'>--.-</span> &deg;C </p>
     </div>
   </div> 
   <div id='commands_section'>     
     <div id='content_commands_section'> 
)====="; 

String html_2 = R"=====(
     </div>
   </div>
 </body>
</html>
)====="; 

/* STEPPER MOTOR PAGE */
String html_stepper_1 = R"=====(
<!DOCTYPE html>
<html>
 <head>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'/>
  <meta charset='utf-8'>
  <style>
    body {
      font-family: Verdana, sans-serif;
      font-size:100%;
    }
    #stepperMotorSection {display: table; margin: auto;  padding: 10px 10px 10px 10px; } 
    #content_stepperMotorSection { border: 5px solid blue; border-radius: 15px; padding: 10px 0px 10px 0px;}

    h2 {text-align:center; margin: 10px 0px 10px 0px;} 
    p { text-align:center; margin: 5px 0px 10px 0px; font-size: 120%;}
    #time_P { margin: 10px 0px 15px 0px;}

    .button {
      border: none;
      padding: 16px 40px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 30px;
      margin: 4px 2px;
      transition-duration: 0.4s;
      cursor: pointer;
    }

   .buttonOn {
      background-color: #81B57B;
      color: black;
      border: 4px solid #13BD00; /* green */
    }

   .buttonOff {
      background-color: #D97E7E;
      color: black;
      border: 4px solid #C20000; /* red */
    }   
    
  .buttonDisabled {
      background-color: #77818C; /* chiaro grey */
      color: black;
      border: 4px solid #40464D; /* scuro grey */
    }  
  </style>
  
  <title>Pagina controllo motore stepper</title>
 </head>
 
 <body>
   <p><a href="/temperatureAndActuatorsControlPage">CONTROLLO TEMPERATURA E ATTUATORI</a></p>
   <div id='stepperMotorSection'>     
     <div id='content_stepperMotorSection'> 
)====="; 

String html_stepper_2 = R"=====(
     </div>
   </div>
 </body>
</html>
)====="; 
/* END STEPPER MOTOR PAGE*/
 
#include <ESP8266WiFi.h>

// NETWORK CONFIGURATION
#define ACCESS_POINT_CONFIGURATION 
//#define TPLINK_CONFIGURATION 

#ifdef ACCESS_POINT_CONFIGURATION
  IPAddress local_IP(192,168,1,2);
  IPAddress gateway(192,168,1,9);
  IPAddress subnet(255,255,255,0);

  const char* ssid     = "ESP8266_WiFi_Network";
  const char* password = "123456789"; //"31678995";
#endif

#ifdef TPLINK_CONFIGURATION
  char ssid[] = "ProFilo_TP_Link_MR600";  //"CasaWiFi_MASTER";//"EOLO_099657"; //"ProFilo_TP_Link_MR600";       //  your network SSID (name)
  char pass[] = "31678995";  //"giganticdiamond684";//"fFwAeHMZV"; //"31678995";                    //  your network password
#endif
WiFiServer server(80);
String request = "";


// CURRENT SELECTION VARAIBLES
bool oneTimeVar_1 = true;
bool stepperMotorControlPage_var = false;
bool temperatureAndActuatorsControlPage_var = false;

bool stepperMotorForward_var = false;
bool stepperMotorBackward_var = false;
bool mainHeaterOn_var = false;
bool auxHeaterOn_var = false;
bool upperFanOn_var = false;
bool lowerFanOn_var = false;

// MEMORY SELECTION VARIABLES
bool stepperMotorForward_varOld = false;
bool stepperMotorBackward_varOld = false;
bool mainHeaterOn_varOld = false;
bool auxHeaterOn_varOld = false;
bool upperFanOn_varOld = false;
bool lowerFanOn_varOld = false;

float sensor1_value, sensor2_value, sensor3_value;

// RECEIVING FROM ARDUINO
#define MAX_NUMBER_OF_COMMANDS_FROM_BOARD 20
bool receivingDataFromBoard = false;
String receivedCommands[MAX_NUMBER_OF_COMMANDS_FROM_BOARD];
byte numberOfCommandsFromBoard;

// SENDING TO ARDUINO
#define MAX_NUMBER_OF_COMMANDS_TO_BOARD 20
String listofDataToSend[MAX_NUMBER_OF_COMMANDS_TO_BOARD];
byte listofDataToSend_numberOfData = 0;

void setup() {
  Serial.begin(9600);

  #ifdef ACCESS_POINT_CONFIGURATION
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(ssid); //senza campo password, così è ad accesso libero
    IPAddress IP = WiFi.softAPIP();
  #endif

  #ifdef TPLINK_CONFIGURATION
    WiFi.begin(ssid, pass);
    while(WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
    //Serial.println(WiFi.localIP()); 
    //Serial.println();
  #endif 
  server.begin();  
  serialFlush();
  delay(5);
}
/*
Il client si connette periodicamente per ricevere i dati di temperatura e aggiornarsi.
Lo fa con ajax: script HTML manda periodicamente una request di get della temperatura.

avere un client significa NON avere qualcuno di connesso, ma significa avere qualche pagina HTML
che fa una richiesta.
Abbiamo quindi un client ogni volta che vengono fatte le richieste di temperatura oppure quando
si premono i pulsanti.

*/
void loop() {
  if(Serial.available() > 0){
    numberOfCommandsFromBoard = readFromBoard('#'); // ending communication character from Arduino

    // smistamento dei comandi/dati da Arduino
    for(byte j = 0; j < numberOfCommandsFromBoard; j++){
      String tempReceivedCommand = receivedCommands[j];
      /* do something here*/
    }
  }


  WiFiClient client = server.available();     // Check if a client has connected

  if(!client){ // if there aren't requests from the HTML web page, then do nothing
    ; 
  }
  else{
    Serial.println('$'); // if code is following this branch, I want to notify Arduino I'm gonna be SLOW
    delay(5);
    if(oneTimeVar_1){
      /* printing first page */
      client.flush();
      client.print(header);
      client.print(html_1); 

      client.print("<p>Riscaldatore Principale: <b style='color:red'>OFF</b></p>");
      client.print("<p><a href=\"/mainHeater/on\"><button class=\"button buttonOn\">ON</button></a></p>");

      client.print("<p>Riscaldatore Ausiliario: <b style='color:red'>OFF</b></p>");
      client.print("<p><a href=\"/auxHeater/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 

      client.print("<p>Ventilatore Superiore: <b style='color:red'>OFF</b></p>");
      client.print("<p><a href=\"/upperFan/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 

      client.print("<p>Ventilatore Inferiore: <b style='color:red'>OFF</b></p>");
      client.print("<p><a href=\"/lowerFan/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 

      client.print(html_2);
      oneTimeVar_1 = false;
    }
    request = client.readStringUntil('\r');     // Read the first line of the request

    if(request.isEmpty()){
      Serial.println('@'); 
    }
    else{
      //Serial.println(request);
      /* HANDLING WEB PAGES REQUESTS */
      if(request.indexOf("getTemp") >= 0){
        sensor1_value = getFloatFromString(receivedCommands[0], ','); 
        sensor2_value = getFloatFromString(receivedCommands[1], ','); 
        sensor3_value = getFloatFromString(receivedCommands[2], ','); 

        if (!isnan(sensor1_value) && !isnan(sensor2_value) && !isnan(sensor3_value)){
            client.print(header);
            client.print(sensor1_value);   client.print( "|" );  client.print(sensor2_value);   client.print( "|" );  client.print(sensor3_value); 
            
        }
        Serial.println('@'); 
        return; // perché se procedessi giù rigenero la pagina HTML da capo
      }

      if(request.indexOf("favicon.ico") >= 0){
        Serial.println('@'); 
        return;
      }

      /* WEB PAGE SELECTION */
      if(request.indexOf("GET /stepperMotorControlPage") >= 0){
        stepperMotorControlPage_var = true; 
      }

      if(request.indexOf("GET /temperatureAndActuatorsControlPage") >= 0){ 
        stepperMotorControlPage_var = false;           
      } 

      /* STEPPER MOTOR CONTROL */
      if(request.indexOf("GET /stepperMotorForward/on") >= 0){
        stepperMotorForward_var = true; 
        //Serial.println("stepperMotorForward_var_true");
      }

      if(request.indexOf("GET /stepperMotorForward/off") >= 0){
        stepperMotorForward_var = false; 
        //Serial.println("stepperMotorForward_var_false");
      }

      if(request.indexOf("GET /stepperMotorBackward/on") >= 0){
        stepperMotorBackward_var = true; 
        //Serial.println("stepperMotorBackward_var_true");
      }

      if(request.indexOf("GET /stepperMotorBackward/off") >= 0){
        stepperMotorBackward_var = false; 
        //Serial.println("stepperMotorBackward_var_false");
      }

      /* MAIN HEATER */
      if(request.indexOf("GET /mainHeater/on") >= 0){
        mainHeaterOn_var = true; 
        //Serial.println("mainHeaterOn_var_true");
      }

      if(request.indexOf("GET /mainHeater/off") >= 0){ 
        mainHeaterOn_var = false;    
        //Serial.println("mainHeaterOn_var_false");       
      } 

      /* AUXILIARY HEATER */
      if(request.indexOf("GET /auxHeater/on") >= 0){
        auxHeaterOn_var = true;
        //Serial.println("auxHeaterOn_var_true");
      }

      if(request.indexOf("GET /auxHeater/off") >= 0){
        auxHeaterOn_var = false;
        //Serial.println("auxHeaterOn_var_false"); 
      }

      /* UPPER FAN */
      if(request.indexOf("GET /upperFan/on") >= 0){
        upperFanOn_var = true;
        //Serial.println("upperFanOn_var_true");
      }

      if(request.indexOf("GET /upperFan/off") >= 0){
        upperFanOn_var = false; 
        //Serial.println("upperFanOn_var_false");
      }  

      /* LOWER FAN */
      if(request.indexOf("GET /lowerFan/on") >= 0){
        lowerFanOn_var = true;
        //Serial.println("lowerFanOn_var_true");
      }      
      
      if(request.indexOf("GET /lowerFan/off") >= 0){
        lowerFanOn_var = false;
        //Serial.println("lowerFanOn_var_false");
      } 

      /* HTML PAGE UPDATE */
      client.flush();
      client.print(header);

      if(stepperMotorControlPage_var){
        client.print(html_stepper_1);
        
        if(stepperMotorForward_var){
          client.print("<p>Stepper Avanti: <b style='color:green'>ON</b></p>");
          client.print("<p><a href=\"/stepperMotorForward/off\"><button class=\"button buttonOff\">OFF</button></a></p>");
          client.print("<p>Stepper Indietro: <b style='color:red'>OFF</b></p>");
          client.print("<p><a href=\"/stepperMotorBackward/on\"><button class=\"button buttonDisabled\" disabled>ON</button></a></p>"); // backward button disabled
        }
        else if(stepperMotorBackward_var){
          client.print("<p>Stepper Avanti: <b style='color:red'>OFF</b></p>");
          client.print("<p><a href=\"/stepperMotorForward/on\"><button class=\"button buttonDisabled\" disabled>ON</button></a></p>"); // forward button disabled
          client.print("<p>Stepper Indietro: <b style='color:green'>ON</b></p>");
          client.print("<p><a href=\"/stepperMotorBackward/off\"><button class=\"button buttonOff\">OFF</button></a></p>"); 
        }
        else{
          client.print("<p>Stepper Avanti: <b style='color:red'>OFF</b></p>");
          client.print("<p><a href=\"/stepperMotorForward/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 
          client.print("<p>Stepper Indietro: <b style='color:red'>OFF</b></p>");
          client.print("<p><a href=\"/stepperMotorBackward/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 
        }
        client.print(html_stepper_2);
      }
      else{
        client.print(html_1);
          /* MAIN HEATER */   
        if(mainHeaterOn_var){
          client.print("<p>Riscaldatore Principale: <b style='color:green'>ON</b></p>");
          client.print("<p><a href=\"/mainHeater/off\"><button class=\"button buttonOff\">OFF</button></a></p>"); 
        }
        else{
          client.print("<p>Riscaldatore Principale: <b style='color:red'>OFF</b></p>");
          client.print("<p><a href=\"/mainHeater/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 
        }

        /* AUXILIARY HEATER */
        if(auxHeaterOn_var){
          client.print("<p>Riscaldatore Ausiliario: <b style='color:green'>ON</b></p>");
          client.print("<p><a href=\"/auxHeater/off\"><button class=\"button buttonOff\">OFF</button></a></p>"); 
        }
        else{
          client.print("<p>Riscaldatore Ausiliario: <b style='color:red'>OFF</b></p>");
          client.print("<p><a href=\"/auxHeater/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 
        }
        
        /* UPPER FAN */
        if(upperFanOn_var){
          client.print("<p>Ventola Superiore: <b style='color:green'>ON</b></p>");
          client.print("<p><a href=\"/upperFan/off\"><button class=\"button buttonOff\">OFF</button></a></p>"); 
        }
        else{
          client.print("<p>Ventola Superiore: <b style='color:red'>OFF</b></p>");
          client.print("<p><a href=\"/upperFan/on\"><button class=\"button buttonOn\">ON</button></a></p>");
        }

        /* LOWER FAN */
        if(lowerFanOn_var){
          client.print("<p>Ventola Inferiore: <b style='color:green'>ON</b></p>");
          client.print("<p><a href=\"/lowerFan/off\"><button class=\"button buttonOff\">OFF</button></a></p>"); 
        }
        else{
          client.print("<p>Ventola Inferiore: <b style='color:red'>OFF</b></p>");
          client.print("<p><a href=\"/lowerFan/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 
        }
        
        client.print(html_2);
      }

      /* commands */

      listofDataToSend_numberOfData = 0;

      if(stepperMotorForward_var && !stepperMotorForward_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<stepperMotorForwardOn>\0";
        listofDataToSend_numberOfData ++;
        stepperMotorForward_varOld = true;
      }
      if(!stepperMotorForward_var && stepperMotorForward_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<stepperMotorForwardOff>";
        listofDataToSend_numberOfData ++;
        stepperMotorForward_varOld = false;
      }

      if(stepperMotorBackward_var && !stepperMotorBackward_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<stepperMotorBackwardOn>";
        listofDataToSend_numberOfData ++;
        stepperMotorBackward_varOld = true;
      }
      if(!stepperMotorBackward_var && stepperMotorBackward_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<stepperMotorBackwardOff>";
        listofDataToSend_numberOfData ++;
        stepperMotorBackward_varOld = false;
      }

      if(mainHeaterOn_var && !mainHeaterOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<mainHeaterOn>";
        listofDataToSend_numberOfData ++;
        mainHeaterOn_varOld = true;
      }
      if(!mainHeaterOn_var && mainHeaterOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<mainHeaterOff>";
        listofDataToSend_numberOfData ++;
        mainHeaterOn_varOld = false;
      }

      if(auxHeaterOn_var && !auxHeaterOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<auxHeaterOn>";
        listofDataToSend_numberOfData ++;
        auxHeaterOn_varOld = true;
      }
      if(!auxHeaterOn_var && auxHeaterOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<auxHeaterOff>";
        listofDataToSend_numberOfData ++;
        auxHeaterOn_varOld = false;
      }

      if(upperFanOn_var && !upperFanOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<upperFanOn>";
        listofDataToSend_numberOfData ++;
        upperFanOn_varOld = true;
      }
      if(!upperFanOn_var && upperFanOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<upperFanOff>";
        listofDataToSend_numberOfData ++;
        upperFanOn_varOld = false;
      }

      if(lowerFanOn_var && !lowerFanOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<lowerFanOn>";
        listofDataToSend_numberOfData ++;
        lowerFanOn_varOld = true;
      }
      if(!lowerFanOn_var && lowerFanOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<lowerFanOff>";
        listofDataToSend_numberOfData ++;
        lowerFanOn_varOld = false;
      }

      serialFlush(); 
      for(byte i = 0; i < listofDataToSend_numberOfData; i++){
        Serial.print(listofDataToSend[i]); 
      }
      Serial.print("@"); // notifying Arduino the end of the transmission
      delay(1);
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
