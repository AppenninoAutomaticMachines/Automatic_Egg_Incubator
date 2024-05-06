#include <ProfiloLibrary.h>

antiDebounceInput Inductor_input(12, 25);
trigger stepperAutomaticControl_trigger;
bool ledVar;

void setup() {
  pinMode(13, OUTPUT);
  pinMode(12, INPUT);

  Serial.begin(9600);

}

void loop() {
  Inductor_input.periodicRun();

  stepperAutomaticControl_trigger.periodicRun(Inductor_input.getInputState());

  if(stepperAutomaticControl_trigger.catchRisingEdge()){
    ledVar = !ledVar;
    digitalWrite(13, ledVar);
  }

  Serial.print(ledVar);
  Serial.print(" ");
  Serial.print(Inductor_input.getInputState());
  Serial.println();


}
