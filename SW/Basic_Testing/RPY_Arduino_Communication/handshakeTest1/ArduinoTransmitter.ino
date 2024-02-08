
int sensor1;
int sensor2;
int sensor3;
int sensor4;

bool digitalInput1;
bool digitalInput2;
bool digitalInput3;
bool digitalInput4;

void setup() {
  // put your setup code here, to run once:
  randomSeed(analogRead(0));
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
 //getCommandsFromRaspberry();

  /* DO THINGS */
  sensor1 = random(10000); // random number from 0 to 1000;
  sensor2 = random(10000); // random number from 0 to 1000;
  sensor3 = random(10000); // random number from 0 to 1000;
  sensor4 = random(10000); // random number from 0 to 1000;

  digitalInput1 = (random(1000) >= 500);
  digitalInput2 = (random(1000) >= 500);
  digitalInput3 = (random(1000) >= 500);
  digitalInput4 = (random(1000) >= 500);

  //writeDataToSerial();
  //Serial.println(256); printLN mette \r = carriage return e \n = new line -> come premere Enter
  printSymbolAndData("Sensor1", sensor1);
  printSymbolAndData("Sensor2", sensor2);
  printSymbolAndData("digitalInput1", digitalInput1);
  delay(250);
}

void printSymbolAndData(String symbolName, int data){
  Serial.print(symbolName);
  Serial.print("_");
  Serial.print(data);
  Serial.print("#");
}
