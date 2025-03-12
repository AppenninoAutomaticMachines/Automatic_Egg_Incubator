/* LIBRARIES */
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <ProfiloLibrary.h>

#define DEFAULT_DEBOUNCE_TIME 25 //ms
#define CCW_INDUCTOR_PIN 9 // induttore finecorsa SINISTRO (vista posteriore)
#define CW_INDUCTOR_PIN 8


/* CCW_LS */
antiDebounceInput ccw_inductor_input(CCW_INDUCTOR_PIN, DEFAULT_DEBOUNCE_TIME);
trigger ccw_trigger;

/* CW_LS */
antiDebounceInput cw_inductor_input(CW_INDUCTOR_PIN, DEFAULT_DEBOUNCE_TIME);
trigger cw_trigger;

void setup() {
  Serial.begin(9600);
  pinMode(CCW_INDUCTOR_PIN, INPUT);
  pinMode(CW_INDUCTOR_PIN, INPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
    /* INDUCTOR INPUT SECTION */
  ccw_inductor_input.periodicRun();
  cw_inductor_input.periodicRun();
  /* END INDUCTOR INPUT SECTION */

  ccw_trigger.periodicRun(ccw_inductor_input.getInputState());
  cw_trigger.periodicRun(cw_inductor_input.getInputState());

  Serial.print("CCW: ");
  Serial.print(ccw_inductor_input.getInputState());
  Serial.print("  CW: ");
  Serial.print(cw_inductor_input.getInputState());
  Serial.println();


}
