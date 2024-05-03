#include <ProfiloLibrary.h>
#define STEPPPER_MOTOR_STEP_PIN 8
#define STEPPPER_MOTOR_DIRECTION_PIN 7
#define STEPPPER_MOTOR_MS1_PIN 6 
#define STEPPPER_MOTOR_MS2_PIN 5
#define STEPPPER_MOTOR_MS3_PIN 4

/* MOTORS SECTION */
#define STEPPER_MOTOR_SPEED_DEFAULT 50 //rpm

float stepper_motor_speed = STEPPER_MOTOR_SPEED_DEFAULT;

stepperMotor eggsTurnerStepperMotor(STEPPPER_MOTOR_STEP_PIN, STEPPPER_MOTOR_DIRECTION_PIN, STEPPPER_MOTOR_MS1_PIN, STEPPPER_MOTOR_MS2_PIN, STEPPPER_MOTOR_MS3_PIN, 1.8);

bool move = false;
bool direction = false; 

bool stepperAutomaticControl_var = false;


// variabile per manual control
bool stepperMotor_moveForward_var = false;
bool stepperMotor_moveBackward_var = false;
bool stepperMotor_stop_var = false;

byte eggsTurnerState = 0;
bool turnEggs_cmd = false;

timer dummyTimer;
/* END MOTORS SECTION */
#define CYCLE_TOGGLE_PIN 3
bool cycle_toggle_pin_var = false;


void setup() {
  pinMode(STEPPPER_MOTOR_STEP_PIN, OUTPUT);
  digitalWrite(STEPPPER_MOTOR_STEP_PIN, LOW);

  pinMode(STEPPPER_MOTOR_DIRECTION_PIN, OUTPUT);
  digitalWrite(STEPPPER_MOTOR_DIRECTION_PIN, LOW);  

  pinMode(STEPPPER_MOTOR_MS1_PIN, OUTPUT);
  digitalWrite(STEPPPER_MOTOR_MS1_PIN, LOW);

  pinMode(STEPPPER_MOTOR_MS2_PIN, OUTPUT);
  digitalWrite(STEPPPER_MOTOR_MS2_PIN, LOW);

  pinMode(STEPPPER_MOTOR_MS3_PIN, OUTPUT);
  digitalWrite(STEPPPER_MOTOR_MS3_PIN, LOW);

  pinMode(CYCLE_TOGGLE_PIN, OUTPUT);
  // put your setup code here, to run once:
  Serial.begin(19200);
  
}

void loop() {
  eggsTurnerStepperMotor.periodicRun();
  eggsTurnerStepperMotor.moveForward(STEPPER_MOTOR_SPEED_DEFAULT);
  // put your main code here, to run repeatedly:
  digitalWrite(CYCLE_TOGGLE_PIN, cycle_toggle_pin_var);
  cycle_toggle_pin_var = !cycle_toggle_pin_var;
   Serial.println();

}
