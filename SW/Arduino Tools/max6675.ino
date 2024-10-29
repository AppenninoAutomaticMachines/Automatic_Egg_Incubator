// sw per leggere la temperatura da due termocoppie

#include "max6675.h"


int thermoDO = 8;
int thermoCS = 9;
int thermoCLK = 10;

int thermoDO2 = 11;
int thermoCS2 = 12;
int thermoCLK2 = 13;



MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
MAX6675 thermocouple2(thermoCLK2, thermoCS2, thermoDO2);


void setup() {
  Serial.begin(9600);


  Serial.println("MAX6675 test");
  // wait for MAX chip to stabilize
  delay(500);
}


void loop() {
  // basic readout test, just print the current temp
 
   Serial.print("C = ");
   Serial.print(thermocouple.readCelsius());
   delay(20);
   Serial.print("  C2 = ");
   Serial.println(thermocouple2.readCelsius());
 
   // For the MAX6675 to update, you must delay AT LEAST 250ms between reads!
   delay(1000);
}
