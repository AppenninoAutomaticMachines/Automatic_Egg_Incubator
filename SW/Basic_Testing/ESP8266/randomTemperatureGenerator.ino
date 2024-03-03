String inString = "";  // string to hold input

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

}

void loop() {
  // put your main code here, to run repeatedly:
  float randT1 = random(100, 500) / 10.0;
  float randT2 = random(100, 500) / 10.0;

  Serial.print("<Temp1,");
  Serial.print(randT1);
  Serial.print(">");
  delay(250);

  Serial.print("<Temp2,");
  Serial.print(randT2);
  Serial.print(">");

  delay(250);
}
