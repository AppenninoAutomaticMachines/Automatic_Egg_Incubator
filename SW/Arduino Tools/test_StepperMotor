byte dirPin = 5;
byte stepPin = 6;

bool clockwise = true;

byte stepCounter = 0;

void setup() {
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(clockwise){
    digitalWrite(dirPin, HIGH);
  }
  else{
    digitalWrite(dirPin, LOW);
  }

  while(stepCounter < 100){
    digitalWrite(stepPin, HIGH);
    delay(1);
    digitalWrite(stepPin, LOW);
    delay(50);
    stepCounter++;
    Serial.println(stepCounter);
  }
  stepCounter = 0;
  clockwise = !clockwise;
  delay(1000);
}
