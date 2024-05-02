#include <ProfiloLibrary.h>
#define DEFAULT_DEBOUNCE_TIME 25 //ms

#define STEPPER_MOTOR_SPEED_DEFAULT 2 //rpm

byte stepPin = 8;
byte directionPin = 7;
byte MS1 = 6;
byte MS2 = 5;
byte MS3 = 4;

bool turnEggs_cmd = false;

timer dummyTimer;

byte eggsTurnerState = 0;

float stepper_motor_speed = STEPPER_MOTOR_SPEED_DEFAULT;

stepperMotor eggsTurnerStepperMotor(stepPin, directionPin, MS1, MS2, MS3, 0.5);

#define LEFT_INDUCTOR_PIN 6
antiDebounceInput leftInductor_input(LEFT_INDUCTOR_PIN, DEFAULT_DEBOUNCE_TIME);

#define RIGHT_INDUCTOR_PIN 5
antiDebounceInput rightInductor_input(RIGHT_INDUCTOR_PIN, DEFAULT_DEBOUNCE_TIME);

void setup() {

    pinMode(stepPin, OUTPUT);
  digitalWrite(stepPin, LOW);

  pinMode(directionPin, OUTPUT);
  digitalWrite(directionPin, LOW);  

  pinMode(MS1, OUTPUT);
  digitalWrite(MS1, LOW);

  pinMode(MS2, OUTPUT);
  digitalWrite(MS2, LOW);

  pinMode(MS3, OUTPUT);
  digitalWrite(MS3, LOW);

   pinMode(LEFT_INDUCTOR_PIN, INPUT);
  pinMode(RIGHT_INDUCTOR_PIN, INPUT);


  Serial.begin(9600);

  dummyTimer.setTimeToWait(10000); //DUMMY FOR TESTS - giro ogni 10 secondi

}

void loop() {
  eggsTurnerStepperMotor.periodicRun();
  leftInductor_input.periodicRun();
  rightInductor_input.periodicRun();
  dummyTimer.periodicRun();


  turnEggs_cmd = dummyTimer.getOutputTriggerEdgeType();
  /*
  switch(eggsTurnerState){
      case 0: // ZEROING
        // lascio stato aperto per eventuale abilitazione della procedura di azzeramento da parte dell'operatore.
        eggsTurnerStepperMotor.moveForward(STEPPER_MOTOR_SPEED_DEFAULT);
        eggsTurnerState = 1;
        break;
      case 1: // WAIT_TO_REACH_LEFT_SIDE_INDUCTOR
        if(leftInductor_input.getInputState()){
          // home position is reached - full left
          eggsTurnerStepperMotor.stopMotor();
          dummyTimer.reset();
          eggsTurnerState = 2;
        }
        break;
      case 2: // WAIT_FOR_TURN_EGGS_COMMAND_STATE --> will rotate from left to right (CW direction)
          if(turnEggs_cmd){
            eggsTurnerStepperMotor.moveBackward(STEPPER_MOTOR_SPEED_DEFAULT);
            eggsTurnerState = 3;
          }
        break;
      case 3: // WAIT_TO_REACH_RIGHT_SIDE_INDUCTOR
          if(rightInductor_input.getInputState()){
            dummyTimer.reArm();
            eggsTurnerStepperMotor.stopMotor();
            eggsTurnerState = 4;
          }
        break;
      case 4: // WAIT_FOR_TURN_EGGS_COMMAND_STATE --> will rotate from right to left (CCW direction)
          if(turnEggs_cmd){
            eggsTurnerStepperMotor.moveForward(STEPPER_MOTOR_SPEED_DEFAULT);
            eggsTurnerState = 5;
          }
        break;
      case 5: // WAIT_TO_REACH_LEFT_SIDE_INDUCTOR
          if(leftInductor_input.getInputState()){
            dummyTimer.reArm();
            eggsTurnerStepperMotor.stopMotor();
            eggsTurnerState = 2;
          }
        break;
      default:
        break;
    }
    */

  // put your main code here, to run repeatedly:
  rightInductor_input.periodicRun();
  Serial.print(leftInductor_input.getInputState());
  Serial.print("  ");
  Serial.print(rightInductor_input.getInputState());
  Serial.print("  ");
  Serial.print(eggsTurnerState);
  Serial.println();

}
