
int sensor1 = 8;
int sensor2 = 9;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  // make the pushbutton's pin an input:
  pinMode(sensor1, INPUT);
  pinMode(sensor2, INPUT);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input pin:
  int buttonState1 = digitalRead(sensor1);
  int buttonState2 = digitalRead(sensor2);
  // print out the state of the button:
  Serial.print(buttonState1);
  Serial.print("    ");
  Serial.println(buttonState2);
  delay(1);  // delay in between reads for stability
}
