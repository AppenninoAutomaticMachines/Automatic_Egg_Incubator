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

/* CONTROLLO MANUALE */
String manualControl_html_1 = R"=====(
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

    .lamp {
      width: 25px;
      height: 25px;
      background-color: #FFFFFF; /* default color white */
      border-radius: 50%;
      transition: background-color 0.5s ease;
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
 
    function updateVariables() 
    {  
       ajaxLoad("getTemp3"); 
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
 
    var myVar1 = setInterval(updateVariables, 750);  
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

 </head>
 
 <body>
   <p id='P_time'>Time</p>
   <p><a href="/mainPage">PAGINA INIZIALE</a></p>
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

String manualControl_html_2 = R"=====(
     </div>
   </div>
 </body>
</html>
)====="; 
/* END CONTROLLO MANUALE */

/* CONTROLLO AUTOMATICO */
String automaticControl_html_1 = R"=====(
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

    #control_variables_section {display: table; margin: auto;  padding: 10px 10px 10px 10px; } 
    #content_control_variables_section { border: 5px solid DarkMagenta; border-radius: 15px; padding: 10px 0px 10px 0px;}

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

    .lamp {
      width: 20px;
      height: 20px;
      background-color: #FFFFFF; /* default color */
      border-radius: 50%;
      margin: auto 0;
    }

    
  </style>
 
  <script> 
    function updateTime(){  
       var d = new Date();
       var t = "";
       t = d.toLocaleTimeString();
       document.getElementById('P_time').innerHTML = t;
    }
 
    function updateVariables(){  
       ajaxLoad("/getVariables"); 
    }
 
    var ajaxRequest = null;

    if (window.XMLHttpRequest){ 
      ajaxRequest =new XMLHttpRequest(); 
    }
    else{ 
      ajaxRequest =new ActiveXObject("Microsoft.XMLHTTP"); 
    }
 
    function ajaxLoad(ajaxURL){
      if(!ajaxRequest){ 
        alert('AJAX is not supported.'); return; 
      }
 
      ajaxRequest.open('GET',ajaxURL,true);
      ajaxRequest.onreadystatechange = function(){
        if(ajaxRequest.readyState == 4 && ajaxRequest.status==200){
          var ajaxResult = ajaxRequest.responseText;
          var tmpArray = ajaxResult.split("|");
          document.getElementById('temp_sensor1').innerHTML = tmpArray[0];
          document.getElementById('temp_sensor2').innerHTML = tmpArray[1];
          document.getElementById('temp_sensor3').innerHTML = tmpArray[2];
          document.getElementById('actualT').innerHTML = tmpArray[3];
          document.getElementById('hHystLimINO').innerHTML = tmpArray[4];
          document.getElementById('lHystLimINO').innerHTML = tmpArray[5];

          var lamp_mainHeater = document.getElementById('lamp_mainHeater');
          if (tmpArray[6] === "1") {
            lamp_mainHeater.style.backgroundColor = '#FF0000'; // color when boolean is true
          } else {
            lamp_mainHeater.style.backgroundColor = '#A0A0A0'; // default color when boolean is false
          }

          var lamp_auxHeater = document.getElementById('lamp_auxHeater');
          if (tmpArray[7] === "1") {
            lamp_auxHeater.style.backgroundColor = '#FF0000'; // color when boolean is true
          } else {
            lamp_auxHeater.style.backgroundColor = '#A0A0A0'; // default color when boolean is false
          }

          document.getElementById('tCM_INO').innerHTML = tmpArray[8];
        }
      }
      ajaxRequest.send();
    }
 
    var myVar1 = setInterval(updateVariables, 500);  
    var myVar2 = setInterval(updateTime, 500);  

    document.addEventListener("DOMContentLoaded", function (event){
        var scrollpos = sessionStorage.getItem('scrollpos');
        if (scrollpos) {
            window.scrollTo(0, scrollpos);
            sessionStorage.removeItem('scrollpos');
        }
    });

    window.addEventListener("beforeunload", function (e){
        sessionStorage.setItem('scrollpos', window.scrollY);
    });

    function submitForm_higherHysteresisLimit() {
        var number = document.getElementById("hHL").value;
        var xhr = new XMLHttpRequest();
        xhr.open("POST", "hHL," + number + ";", true);
        xhr.send();
    }

    function submitForm_lowerHysteresisLimit() {
        var number = document.getElementById("lHL").value;
        var xhr = new XMLHttpRequest();
        xhr.open("POST", "lHL," + number + ";", true);
        xhr.send();
    }

    function submitForm_temperatureControlModality() {
        var number = document.getElementById("tCM").value;
        var xhr = new XMLHttpRequest();
        xhr.open("POST", "tCM," + number + ";", true);
        xhr.send();
    }


    var mainHeaterOn_html = false;
    var auxHeaterOn_html = false;

    function updateLampColor() {
      var lamp_mainHeater = document.getElementById('lamp_mainHeater');
      if (lamp_mainHeater === "1") {
        lamp_mainHeater.style.backgroundColor = '#FF0000'; // color when boolean is true
      } else {
        lamp_mainHeater.style.backgroundColor = '#A0A0A0'; // default color when boolean is false
      }

      var lamp_auxHeater = document.getElementById('lamp_auxHeater');
      if (lamp_auxHeater === "1") {
        lamp_auxHeater.style.backgroundColor = '#FF0000'; // color when boolean is true
      } else {
        lamp_auxHeater.style.backgroundColor = '#A0A0A0'; // default color when boolean is false
      }
    }

    // Call the function to set the initial state
    updateLampColor();
  </script>

 </head>
 
 <body>
   <p id='P_time'>Time</p>
   <p><a href="/mainPage">PAGINA INIZIALE</a></p>
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
    <div id='control_variables_section'>     
     <div id='content_control_variables_section'> 
       <h2>T controllo:</h2>
       <p> <span id='actualT'>--.-</span> &deg;C </p>
       <div style="display: flex;">
        <p>Risc Principale: </p> 
        <div class="lamp" id="lamp_mainHeater"></div>
       </div>
       <div style="display: flex;">
        <p>Risc Ausiliario: </p> 
        <div class="lamp" id="lamp_auxHeater"></div>
       </div>
       <form id="numberForm">
          <label for="hHL">T Alta:</label><br>
          <input type="number" id="hHL" name="hHL">
          <span id='hHystLimINO'>--.-</span> &deg;C
          <br><br>
          <button type="button" onclick="submitForm_higherHysteresisLimit()">Submit</button>
          <br><br>

          <label for="lHL">T Bassa:</label><br>
          <input type="number" id="lHL" name="lHL">
          <span id='lHystLimINO'>--.-</span> &deg;C
          <br><br>
          <button type="button" onclick="submitForm_lowerHysteresisLimit()">Submit</button>
          <br><br>

          <label for="tCM">Modalità controllo:</label><br>
          <input type="number" id="tCM" name="tCM">
          <span id='tCM_INO'></span>
          <br><br>
          <button type="button" onclick="submitForm_temperatureControlModality()">Submit</button>
          <br><br>
      </form>
     </div>
    </div>
   <div id='commands_section'>     
     <div id='content_commands_section'> 
)====="; 

String automaticControl_html_2 = R"=====(
     </div>
   </div>
 </body>
</html>
)====="; 
/* END CONTROLLO AUTOMATICO */



/* STEPPER MOTOR PAGE */
String stepperControl_html_1 = R"=====(
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
   <p><a href="/mainPage">PAGINA INIZIALE</a></p>
   <div id='stepperMotorSection'>     
     <div id='content_stepperMotorSection'> 
)====="; 

String stepperControl_html_2 = R"=====(
     </div>
   </div>
 </body>
</html>
)====="; 
/* END STEPPER MOTOR PAGE*/
 
 
 /* MAIN PAGE */
 String mainPage_html_1 = R"=====(
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
  <title>Interfaccia Incubatrice Automatica</title>
 </head>
 
 <body>
   <p id='P_time'>Time</p>
   <p><a href="/stepperMotorControlPage">CONTROLLO MOTORE STEPPER</a></p>
   <p><a href="/automaticControlPage">CONTROLLO AUTOMATICO</a></p>
   <p><a href="/manualControlPage">CONTROLLO MANUALE</a></p>
    </body>
</html>
)====="; 
 /* END MAIN PAGE */
 
#include <ESP8266WiFi.h>

/* ADDITIONAL SERIAL BUS */
#include <SoftwareSerial.h>
#define SERIAL_SPEED 19200 

const byte rxPin = 12;
const byte txPin = 16;
SoftwareSerial toArduinoSerial(rxPin, txPin);

// Set up a new SoftwareSerial object
SoftwareSerial mySerial (rxPin, txPin);
/* END ADDITIONAL SERIAL BUS */

/* NETWORK CONFIGURATION */
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
/* END NETWORK CONFIGURATION */


/* WEB PAGE SELECTION VARAIBLES */
enum web_page_selection_enum{
  MAIN_PAGE,
  STEPPER_MOTOR_CONTROL_PAGE, 
  AUTOMATIC_CONTROL_PAGE,
  MANUAL_CONTROL_PAGE
};

web_page_selection_enum web_page_selection = MAIN_PAGE;




bool oneTimeVar_1 = true;
bool mainPage_var = true;
bool stepperMotorControlPage_var = false;
bool automaticControlPage_var = false;
bool manualControlPage_var = false;

bool automaticControl_var = false; // variabile che attiva o no il controllo automatico
bool automaticControl_varOld = false;

bool stepperMotorForward_var = false;
bool stepperMotorForward_varOld = false;

bool stepperMotorBackward_var = false;
bool stepperMotorBackward_varOld = false;

bool mainHeaterOn_var = false;
bool mainHeaterOn_varOld = false;

bool auxHeaterOn_var = false;
bool auxHeaterOn_varOld = false;

bool upperFanOn_var = false;
bool upperFanOn_varOld = false;

bool lowerFanOn_var = false;
bool lowerFanOn_varOld = false;

float sensor1_value, sensor2_value, sensor3_value, actualTemperature_value;
float higherHysteresisLimit_arduino_html, lowerHysteresisLimit_arduino_html;
float higherHysteresisLimit_user_html, lowerHysteresisLimit_user_html; 
int temperatureControlModality_user_html;
int mainHeaterOn_fromArduino_toHTML, auxHeaterOn_fromArduino_toHTML, temperatureControlModality_fromArduino_toHTML;


bool send_higherHysteresisLimit_user_html = false;
bool send_lowerHysteresisLimit_user_html = false;
bool send_temperatureControlModality_user_html = false;

// RECEIVING FROM ARDUINO
#define MAX_NUMBER_OF_COMMANDS_FROM_BOARD 20
bool receivingDataFromBoard = false;
String receivedCommands[MAX_NUMBER_OF_COMMANDS_FROM_BOARD];
byte numberOfCommandsFromBoard;

// SENDING TO ARDUINO
#define MAX_NUMBER_OF_COMMANDS_TO_BOARD 20
String listofDataToSend[MAX_NUMBER_OF_COMMANDS_TO_BOARD];
byte listofDataToSend_numberOfData = 0;

char bufferChar[35];
char fbuffChar[10];

void setup() {
  Serial.begin(SERIAL_SPEED);

  toArduinoSerial.begin(9600);

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
    numberOfCommandsFromBoard = readFromBoard(); // ending communication character from Arduino

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
    //Serial.println('$'); // if code is following this branch, I want to notify Arduino I'm gonna be SLOW
    if(oneTimeVar_1){
      /* printing first page */
      client.flush();
      client.print(header);
      client.print(mainPage_html_1);
      oneTimeVar_1 = false;
    }
    request = client.readStringUntil('\r');     // Read the first line of the request

    if(request.isEmpty()){
      //Serial.println('@'); 
    }
    else{
      //Serial.println(request);
      /* HANDLING WEB PAGES REQUESTS */
      if(request.indexOf("getTemp3") >= 0){
        sensor1_value = getFloatFromString(receivedCommands[0], ','); 
        sensor2_value = getFloatFromString(receivedCommands[1], ','); 
        sensor3_value = getFloatFromString(receivedCommands[2], ','); 

        if (!isnan(sensor1_value) && !isnan(sensor2_value) && !isnan(sensor3_value)){
            client.print(header);
            client.print(sensor1_value);   client.print( "|" );  client.print(sensor2_value);   client.print( "|" );  client.print(sensor3_value);
            
        }
        //Serial.println('@'); 
        return; // perché se procedessi giù rigenero la pagina HTML da capo
      }

      if(request.indexOf("getVariables") >= 0){
        sensor1_value = getFloatFromString(receivedCommands[0], ','); //<t1, 0.00>
        sensor2_value = getFloatFromString(receivedCommands[1], ','); //<t2, 0.00>
        sensor3_value = getFloatFromString(receivedCommands[2], ','); //<t3, 0.00>
        actualTemperature_value = getFloatFromString(receivedCommands[3], ','); //<aT, 0.00>
        higherHysteresisLimit_arduino_html = getFloatFromString(receivedCommands[4], ','); //<hHL, 0.00>
        lowerHysteresisLimit_arduino_html = getFloatFromString(receivedCommands[5], ','); //<lHL, 0.00>
        mainHeaterOn_fromArduino_toHTML = getIntFromString(receivedCommands[6], ','); // <mHO, 0> receivedCommands[6].substring(6, 11); //<mHO, false> [6:10]
        auxHeaterOn_fromArduino_toHTML = getIntFromString(receivedCommands[7], ',');//<aHO, true>
        temperatureControlModality_fromArduino_toHTML = getIntFromString(receivedCommands[8], ',');//<tCM, 1>

        if (!isnan(sensor1_value) && !isnan(sensor2_value) && !isnan(sensor3_value) && !isnan(actualTemperature_value)){
            client.print(header);
            client.print(sensor1_value);   
            client.print( "|" );  client.print(sensor2_value);   
            client.print( "|" );  client.print(sensor3_value);  
            client.print( "|" );  client.print(actualTemperature_value);
            client.print( "|" );  client.print(higherHysteresisLimit_arduino_html); 
            client.print( "|" );  client.print(lowerHysteresisLimit_arduino_html);
            client.print( "|" );  client.print(mainHeaterOn_fromArduino_toHTML);
            client.print( "|" );  client.print(auxHeaterOn_fromArduino_toHTML);
            client.print( "|" );  client.print(temperatureControlModality_fromArduino_toHTML);
            
        }
        
        //Serial.println('@'); 
        return; // perché se procedessi giù rigenero la pagina HTML da capo
      }

      if(request.indexOf("hHL") >= 0){ // higherHysteresisLimit il formato è del tipo: hHL,20;   , + numero + termino con ;
        higherHysteresisLimit_user_html = getFloatFromStringHtmlPage(request); 
        send_higherHysteresisLimit_user_html = true;
      }

      if(request.indexOf("lHL") >= 0){ // lowerHysteresisLimit il formato è del tipo: POST /lHL,25;
        lowerHysteresisLimit_user_html = getFloatFromStringHtmlPage(request); 
        send_lowerHysteresisLimit_user_html = true;
      }

      if(request.indexOf("tCM") >= 0){ //POST /tCM,2;
        temperatureControlModality_user_html = getIntFromStringHtmlPage(request); 
        send_temperatureControlModality_user_html = true;
      }

      if(request.indexOf("favicon.ico") >= 0){
        //Serial.println('@'); 
        return;
      }

      /* WEB PAGE SELECTION */
      if(request.indexOf("GET /mainPage") >= 0){
        web_page_selection = MAIN_PAGE;
      }

      if(request.indexOf("GET /stepperMotorControlPage") >= 0){
        web_page_selection = STEPPER_MOTOR_CONTROL_PAGE;
      }

      if(request.indexOf("GET /automaticControlPage") >= 0){
        web_page_selection = AUTOMATIC_CONTROL_PAGE;
      }

      if(request.indexOf("GET /manualControlPage") >= 0){
        web_page_selection = MANUAL_CONTROL_PAGE;
      }
      /* END - WEB PAGE SELECTION */

      /* COMMANDS/REQUEST SECTION */
      if(request.indexOf("GET /automaticControlPage/automaticControl/on") >= 0){
        automaticControl_var = true; 
      }

      if(request.indexOf("GET /automaticControlPage/automaticControl/off") >= 0){
        automaticControl_var = false; 
      }


      /* STEPPER MOTOR CONTROL */
      if(request.indexOf("GET /stepperMotorControlPage/stepperMotorForward/on") >= 0){
        stepperMotorForward_var = true; 
        //Serial.println("stepperMotorForward_var_true");
      }

      if(request.indexOf("GET /stepperMotorControlPage/stepperMotorForward/off") >= 0){
        stepperMotorForward_var = false; 
        //Serial.println("stepperMotorForward_var_false");
      }

      if(request.indexOf("GET /stepperMotorControlPage/stepperMotorBackward/on") >= 0){
        stepperMotorBackward_var = true; 
        //Serial.println("stepperMotorBackward_var_true");
      }

      if(request.indexOf("GET /stepperMotorControlPage/stepperMotorBackward/off") >= 0){
        stepperMotorBackward_var = false; 
        //Serial.println("stepperMotorBackward_var_false");
      }

      /* MAIN HEATER */
      if(request.indexOf("GET /manualControlPage/mainHeater/on") >= 0){
        mainHeaterOn_var = true; 
        //Serial.println("mainHeaterOn_var_true");
      }

      if(request.indexOf("GET /manualControlPage/mainHeater/off") >= 0){ 
        mainHeaterOn_var = false;    
        //Serial.println("mainHeaterOn_var_false");       
      } 

      /* AUXILIARY HEATER */
      if(request.indexOf("GET /manualControlPage/auxHeater/on") >= 0){
        auxHeaterOn_var = true;
        //Serial.println("auxHeaterOn_var_true");
      }

      if(request.indexOf("GET /manualControlPage/auxHeater/off") >= 0){
        auxHeaterOn_var = false;
        //Serial.println("auxHeaterOn_var_false"); 
      }

      /* UPPER FAN */
      if(request.indexOf("GET /manualControlPage/upperFan/on") >= 0){
        upperFanOn_var = true;
        //Serial.println("upperFanOn_var_true");
      }

      if(request.indexOf("GET /manualControlPage/upperFan/off") >= 0){
        upperFanOn_var = false; 
        //Serial.println("upperFanOn_var_false");
      }  

      /* LOWER FAN */
      if(request.indexOf("GET /manualControlPage/lowerFan/on") >= 0){
        lowerFanOn_var = true;
        //Serial.println("lowerFanOn_var_true");
      }      
      
      if(request.indexOf("GET /manualControlPage/lowerFan/off") >= 0){
        lowerFanOn_var = false;
        //Serial.println("lowerFanOn_var_false");
      } 




      /* HTML PAGE UPDATE */
      client.flush();
      client.print(header);

      switch(web_page_selection){
        case MAIN_PAGE:
          client.print(mainPage_html_1);
          break;
        case STEPPER_MOTOR_CONTROL_PAGE:
          client.print(stepperControl_html_1);        
          if(stepperMotorForward_var){
            client.print("<p>Stepper Avanti: <b style='color:green'>ON</b></p>");
            client.print("<p><a href=\"/stepperMotorControlPage/stepperMotorForward/off\"><button class=\"button buttonOff\">OFF</button></a></p>");
            client.print("<p>Stepper Indietro: <b style='color:red'>OFF</b></p>");
            client.print("<p><a href=\"/stepperMotorControlPage/stepperMotorBackward/on\"><button class=\"button buttonDisabled\" disabled>ON</button></a></p>"); // backward button disabled
          }
          else if(stepperMotorBackward_var){
            client.print("<p>Stepper Avanti: <b style='color:red'>OFF</b></p>");
            client.print("<p><a href=\"/stepperMotorControlPage/stepperMotorForward/on\"><button class=\"button buttonDisabled\" disabled>ON</button></a></p>"); // forward button disabled
            client.print("<p>Stepper Indietro: <b style='color:green'>ON</b></p>");
            client.print("<p><a href=\"/stepperMotorControlPage/stepperMotorBackward/off\"><button class=\"button buttonOff\">OFF</button></a></p>"); 
          }
          else{
            client.print("<p>Stepper Avanti: <b style='color:red'>OFF</b></p>");
            client.print("<p><a href=\"/stepperMotorControlPage/stepperMotorForward/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 
            client.print("<p>Stepper Indietro: <b style='color:red'>OFF</b></p>");
            client.print("<p><a href=\"/stepperMotorControlPage/stepperMotorBackward/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 
          }
          client.print(stepperControl_html_2);
          break;


        case AUTOMATIC_CONTROL_PAGE:
          client.print(automaticControl_html_1);
            /* AUTOMATIC CONTROL ACTIVATION/DEACTIVATION button */ 
          if(automaticControl_var){
            client.print("<p>Controllo automatico: <b style='color:green'>ON</b></p>");
            client.print("<p><a href=\"/automaticControlPage/automaticControl/off\"><button class=\"button buttonOff\">OFF</button></a></p>"); 
          }
          else{
            client.print("<p>Controllo automatico: <b style='color:red'>OFF</b></p>");
            client.print("<p><a href=\"/automaticControlPage/automaticControl/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 
          }          

          /* ACTUAL TEMPERATURE DISPLAY - per dire qual è la temperatura che sta utilizzando come variabile di controllo */

          /* ACTUATOR STATE DISPLAY */
          client.print(automaticControl_html_2);
          break;


        case MANUAL_CONTROL_PAGE:
          client.print(manualControl_html_1);
            /* MAIN HEATER */   
          if(mainHeaterOn_var){
            client.print("<p>Riscaldatore Principale: <b style='color:green'>ON</b></p>");
            client.print("<p><a href=\"/manualControlPage/mainHeater/off\"><button class=\"button buttonOff\">OFF</button></a></p>"); 
          }
          else{
            client.print("<p>Riscaldatore Principale: <b style='color:red'>OFF</b></p>");
            client.print("<p><a href=\"/manualControlPage/mainHeater/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 
          }

          /* AUXILIARY HEATER */
          if(auxHeaterOn_var){
            client.print("<p>Riscaldatore Ausiliario: <b style='color:green'>ON</b></p>");
            client.print("<p><a href=\"/manualControlPage/auxHeater/off\"><button class=\"button buttonOff\">OFF</button></a></p>"); 
          }
          else{
            client.print("<p>Riscaldatore Ausiliario: <b style='color:red'>OFF</b></p>");
            client.print("<p><a href=\"/manualControlPage/auxHeater/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 
          }
          
          /* UPPER FAN */
          if(upperFanOn_var){
            client.print("<p>Ventola Superiore: <b style='color:green'>ON</b></p>");
            client.print("<p><a href=\"/manualControlPage/upperFan/off\"><button class=\"button buttonOff\">OFF</button></a></p>"); 
          }
          else{
            client.print("<p>Ventola Superiore: <b style='color:red'>OFF</b></p>");
            client.print("<p><a href=\"/manualControlPage/upperFan/on\"><button class=\"button buttonOn\">ON</button></a></p>");
          }

          /* LOWER FAN */
          if(lowerFanOn_var){
            client.print("<p>Ventola Inferiore: <b style='color:green'>ON</b></p>");
            client.print("<p><a href=\"/manualControlPage/lowerFan/off\"><button class=\"button buttonOff\">OFF</button></a></p>"); 
          }
          else{
            client.print("<p>Ventola Inferiore: <b style='color:red'>OFF</b></p>");
            client.print("<p><a href=\"/manualControlPage/lowerFan/on\"><button class=\"button buttonOn\">ON</button></a></p>"); 
          }          
          client.print(manualControl_html_2);
          break;
        default:
          break;
      }
      
      /* COMMAND TO ARDUINO */
      /* LIST:
      STP01 = stepperMotorForwardOn
      STP02 = stepperMotorForwardOff
      STP03 = stepperMotorBackwardOn
      STP04 = stepperMotorBackwardOff

      TMP01 = mainHeaterOn
      TMP02 = mainHeaterOff
      TMP03 = auxHeaterOn
      TMP04 = auxHeaterOff
      TMP05 = higherHysteresisLimit from ESP8266 HMTL page
      TMP06 = lowerHysteresisLimit from ESP8266 HMTL page
      TMP07 = temperatureControlModality value from ESP8226 HTML page

      ACT01 = upperFanOn
      ACT02 = upperFanOff
      ACT03 = lowerFanOn
      ACT04 = lowerFanOff

      GNR01 = automaticControlOn
      GNR02 = automaticControlOff
      
      */

      listofDataToSend_numberOfData = 0;

      if(stepperMotorForward_var && !stepperMotorForward_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<STP01>"; //stepperMotorForwardOn
        listofDataToSend_numberOfData ++;
        stepperMotorForward_varOld = true;
      }
      if(!stepperMotorForward_var && stepperMotorForward_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<STP02>"; //stepperMotorForwardOff
        listofDataToSend_numberOfData ++;
        stepperMotorForward_varOld = false;
      }

      if(stepperMotorBackward_var && !stepperMotorBackward_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<STP03>"; //stepperMotorBackwardOn
        listofDataToSend_numberOfData ++;
        stepperMotorBackward_varOld = true;
      }
      if(!stepperMotorBackward_var && stepperMotorBackward_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<STP04>"; //stepperMotorBackwardOff
        listofDataToSend_numberOfData ++;
        stepperMotorBackward_varOld = false;
      }

      if(mainHeaterOn_var && !mainHeaterOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<TMP01>"; //mainHeaterOn
        listofDataToSend_numberOfData ++;
        mainHeaterOn_varOld = true;
      }
      if(!mainHeaterOn_var && mainHeaterOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<TMP02>"; //mainHeaterOff
        listofDataToSend_numberOfData ++;
        mainHeaterOn_varOld = false;
      }

      if(auxHeaterOn_var && !auxHeaterOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<TMP03>"; //auxHeaterOn
        listofDataToSend_numberOfData ++;
        auxHeaterOn_varOld = true;
      }
      if(!auxHeaterOn_var && auxHeaterOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<TMP04>"; //auxHeaterOff
        listofDataToSend_numberOfData ++;
        auxHeaterOn_varOld = false;
      }

      if(upperFanOn_var && !upperFanOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<ACT01>"; //upperFanOn
        listofDataToSend_numberOfData ++;
        upperFanOn_varOld = true;
      }
      if(!upperFanOn_var && upperFanOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<ACT02>"; //upperFanOff
        listofDataToSend_numberOfData ++;
        upperFanOn_varOld = false;
      }

      if(lowerFanOn_var && !lowerFanOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<ACT03>"; //lowerFanOn
        listofDataToSend_numberOfData ++;
        lowerFanOn_varOld = true;
      }
      if(!lowerFanOn_var && lowerFanOn_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<ACT04>"; //lowerFanOff
        listofDataToSend_numberOfData ++;
        lowerFanOn_varOld = false;
      }

      if(automaticControl_var && !automaticControl_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<GNR01>"; //automaticControlOn
        listofDataToSend_numberOfData ++;
        automaticControl_varOld = true;
      }
      if(!automaticControl_var && automaticControl_varOld){
        listofDataToSend[listofDataToSend_numberOfData] = "<GNR02>"; //automaticControlOff
        listofDataToSend_numberOfData ++;
        automaticControl_varOld = false;
      }

      if(send_higherHysteresisLimit_user_html){
        strcpy(bufferChar, "<TMP05, "); //higherHysteresisLimit from ESP8266 HMTL page
        dtostrf(higherHysteresisLimit_user_html, 1, 1, fbuffChar); 
        listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(strcat(bufferChar, fbuffChar), ">"), "\0");
        listofDataToSend_numberOfData++;
        send_higherHysteresisLimit_user_html = false;
      }

      if(send_lowerHysteresisLimit_user_html){
        strcpy(bufferChar, "<TMP06, "); //lowerysteresisLimit from ESP8266 HMTL page
        dtostrf(lowerHysteresisLimit_user_html, 1, 1, fbuffChar); 
        listofDataToSend[listofDataToSend_numberOfData] = strcat(strcat(strcat(bufferChar, fbuffChar), ">"), "\0");
        listofDataToSend_numberOfData++;
        send_lowerHysteresisLimit_user_html = false;
      }

      if(send_temperatureControlModality_user_html){
        char bufferCharArray[25];	
        byte charCount = sprintf(bufferCharArray, "<TMP07, %d>", temperatureControlModality_user_html); //sprintf function returns the number of characters written to the array
        listofDataToSend[listofDataToSend_numberOfData] = bufferCharArray;
        listofDataToSend_numberOfData++;
      }

      if(listofDataToSend_numberOfData > 0){
        Serial.print("#"); // SYMBOL TO START BOARDS TRANSMISSION
        for(byte i = 0; i < listofDataToSend_numberOfData; i++){
          Serial.print(listofDataToSend[i]); 
        }
        Serial.print("@"); // SYMBOL TO END BOARDS TRANSMISSION
      }
    }
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

int getIntFromStringHtmlPage(String string){ //inputNumber, 23;
  int index;
  byte firstIndex;
  bool memorize = false;
  const byte numChars = 5;
  char receivedChars[numChars];   // an array to store the received data
  byte index_receivedChars = 0;

  for(byte i =0; i < string.length(); i++) {
    char c = string[i];
    
    if(c == ','){
      firstIndex = index;
      memorize = true;
    }
    else if(c == ';'){
      byte next = firstIndex + 1; // posto successivo alla ,
      if(index == next){
        return 0;
      }
      break;
    }
    else{
      if(memorize){
        receivedChars[index_receivedChars] = c;
        index_receivedChars = index_receivedChars + 1;
      }
    }
  }
  return atoi(receivedChars);  
}

float getFloatFromStringHtmlPage(String string){ //inputNumber, 23;
  int index;
  byte firstIndex;
  bool memorize = false;
  const byte numChars = 5;
  char receivedChars[numChars];   // an array to store the received data
  byte index_receivedChars = 0;

  for(byte i =0; i < string.length(); i++) {
    char c = string[i];
    
    if(c == ','){
      firstIndex = index;
      memorize = true;
    }
    else if(c == ';'){
      byte next = firstIndex + 1; // posto successivo alla ,
      if(index == next){
        return 0;
      }
      break;
    }
    else{
      if(memorize){
        receivedChars[index_receivedChars] = c;
        index_receivedChars = index_receivedChars + 1;
      }
    }
  }
  return atof(receivedChars);  
}

void serialFlush(){
  while(Serial.available() > 0) { 
    char t = Serial.read();
  }
}
