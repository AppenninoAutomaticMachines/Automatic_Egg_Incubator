#include "Arduino.h"
#include "ProfiloLibrary.h"

/* TEMPERATURE CONTROLLER CLASS */
#define DEVICE_DISCONNECTED_C -127
#define DEFAULT_TEMPERATURE_C 0

temperatureController::temperatureController(){
  _referenceTemperature = DEFAULT_REFERENCE_TEMPERATURE; // 37.5°C
  _higherTemperature = DEFAULT_REFERENCE_TEMPERATURE + DEFAULT_HYSTERESIS_RANGE; // 39.5°C
  _lowerTemperature = DEFAULT_REFERENCE_TEMPERATURE - DEFAULT_HYSTERESIS_RANGE; // 35.5°C

  _outputState = false; // not heating   
  _controlModality = 1; // temperature control based on the HIGHEST value
  _hysteresisState = 0;

}

// aggiungi eventuale costruttore dove gli passi subito il pin che deve controllare del/dei riscaldatori
// in tal caso metto anche due metodini per fare la forzatura ON OFF dell'attuatore, per fare override del comando automatico in caso di necessità.


void temperatureController::periodicRun(float *temperatures, byte dimension){  
  switch(_controlModality){ // qui controlliamo l'output
    case 0: 
      break;


    case 1: // tratto sopra di heating
      if (dimension > 0){
        float maxTemperature = temperatures[0];
        for (byte i = 1; i < dimension; i++){
          if (temperatures[i] > maxTemperature){
            maxTemperature = temperatures[i];
          }
        }
        _actualTemperature = maxTemperature;
      }
      else{
        // Handle the case when the array is empty
        // Set _actualTemperature to a default value or handle it accordingly
        _actualTemperature = DEFAULT_TEMPERATURE_C;
      }
      break;

    case 2: // tratto sotto di not heating
      float sum = 0.0;
		  byte goodTemperaturesCounter = 0;		
      for (int i = 0; i < dimension; i++){
        float tempC = temperatures[i];

        if (tempC != DEVICE_DISCONNECTED_C){
          sum += tempC;
          goodTemperaturesCounter++;
        } 
        else{
          // Handle the case when a temperature reading is invalid
          // This could include fault handling or logging
        }
      }

      // Check if there are valid temperatures before computing the mean
      if (goodTemperaturesCounter > 0){
        _actualTemperature = sum / goodTemperaturesCounter;
      } 
      else{
        // Handle the case when no valid temperatures are available
        // This could include setting _actualTemperature to a default value
        _actualTemperature = DEFAULT_TEMPERATURE_C;
      }
		break;

    default:
      break;
  }
  // da qui ho la _actualTemperature

  _outputState = false; // not heating  
  switch(_hysteresisState){ // qui controlliamo l'output
    case 0: // primo stato, appena accendo. Dobbiamo definire dove siamo nel ciclo di isteresi.
      if(_actualTemperature < _lowerTemperature){
        _hysteresisState = 1;
      }
      else if(_actualTemperature > _higherTemperature){
        _hysteresisState = 2;              
      }
      else{ // se sono nel mezzo dell'intervallo, NON heating tratto sotto
        _hysteresisState = 2;
      }
      break;


    case 1: // tratto sopra di heating
      _outputState = true; //heating 

      if(_actualTemperature > _higherTemperature){
        _hysteresisState = 2;     
      }
      break;

    case 2: // tratto sotto di not heating
      _outputState = false; //not heating 
        
      if(_actualTemperature < _lowerTemperature){
        _hysteresisState = 1;     
      }
      break;

    default:
      break;
  }

  /* TIMING COUNT: conteggio di quanto sta ON l'attuatore. */
  
}

void temperatureController::setTemperatureHysteresis(float lowerTemperature, float higherTemperature){
  _higherTemperature = higherTemperature;
  _lowerTemperature = lowerTemperature;
}

void temperatureController::setControlModality(int controlModality){
  _controlModality = controlModality;
}

bool temperatureController::getOutputState(void){
  return _outputState;
}

float temperatureController::debug_getActualTemperature(void){
  return _actualTemperature;
}

int temperatureController::debug_getHysteresisState(void){
  return _hysteresisState;
}


/* TIMER CLASS */
#define DEFAULT_WAITING_TIME 1000 //ms

timer::timer(){
  _currentTime = millis();
  _lastTriggerTime = millis();

  _timeToWait = DEFAULT_WAITING_TIME; 
  _enable = true; // di default parto con timer abilitato
  _outputTrigger_edgeType = false;
  _outputTrigger_stableType = false;

  _resetTimer = false;
  _reArm = false;
  
  _first_periodicRun = true;
}

void timer::periodicRun(void){
  if(_first_periodicRun){
    // aggiorniamo il _lastTriggerTime per la prima volta
    _lastTriggerTime = millis();
    _first_periodicRun = false;
  }
  
  if(_enable){
    _outputTrigger_edgeType = false;
	
	if(_resetTimer){
        _lastTriggerTime = _currentTime;
		_outputTrigger_stableType = false;
		_outputTrigger_edgeType = false;
        _resetTimer = false;
      }


    if(!_outputTrigger_stableType){
      /*
        Fintanto che non ho dato il trigger di output faccio contare il timer. Appena l'ho dato, ho bisogno che ci sia un qualcuno che esternamente
        ri-armi il trigger così che possa tornare a contare. Dall'esterno qualcuno mette reArm = true

        Ricorda che ci sono le due uscite del timer. Una che sta su finché non riparto a contare, una che funziona ad edge e sta su per un ciclo PLC.
      */
      _currentTime = millis();

      
      _outputTrigger_stableType = false;

      if((_currentTime - _lastTriggerTime) >= _timeToWait){
        _lastTriggerTime = _currentTime; // let's remember the last time the time has triggered its output

        _outputTrigger_edgeType = true;
        _outputTrigger_stableType = true;
      }
    }
    else{ 
      /*
        Questa è la fase di ri-armo del trigger.
        Mi salvo il tempo attuale e inizio a contare di nuovo. Metto anche a false il bit di riarmo
      */  
      if(_reArm){ // appena ho il comando di riarmo, inizio a contare di nuovo.
        _lastTriggerTime = millis();
        _outputTrigger_stableType = false;
        _reArm = false;
      }
    }      
  }
  else{
    _lastTriggerTime = millis();
    _outputTrigger_stableType = false;
    _reArm = false;
  }
} 

void timer::setTimeToWait(int timeToWait){
  _timeToWait = timeToWait;
}

void timer::reArm(void){
  _reArm = true;
}

void timer::enable(void){
  _enable = true;
}

void timer::disable(void){
  _enable = false;
}

void timer::reset(void){
  _resetTimer = true;
}

bool timer::getOutputTriggerEdgeType(void){
  return _outputTrigger_edgeType;
}

bool timer::getOutputTriggerStableType(void){
  return _outputTrigger_stableType;
}

/* STEPPER MOTOR CLASS */

stepperMotor::stepperMotor(byte stepPin, byte dirPin, byte MS1, byte MS2, byte MS3, float degreePerStep){
  _stepPin = stepPin;
  _stepCommand = false;

  _dirPin = dirPin;
  _dirCommand = false;
  _switchPolarity = false;

  _MS1 = MS1;
  _MS2 = MS2;
  _MS3 = MS3;

  _rpm_speed = 1; // default 1rpm
  _degreePerStep = degreePerStep;

  _steppingInProgress = false;

  _steppingTimer_stepOn.setTimeToWait(1); // tempo minimo per sui sta su il segnale di step

  _workingState = 0; // STOP
}

void stepperMotor::periodicRun(void){

  /* macchina a stati di gestione dei comandi */
  switch(_workingState){
    case 0: // STOP
      _stepCommand = false;
      break;

    case 1:// MOVE FORWARD
      _stepCommand = true;
      _dirCommand = true;
      break;

    case 2:// MOVE BACKWARD
      _stepCommand = true;
      _dirCommand = false;
      break;

    default:
      break;
  }


  if(_stepCommand){
    // scelgo direzione e abilito i timer
    if(_switchPolarity){
      if(_dirCommand){
        digitalWrite(_dirPin, LOW);
      }
      else{
        digitalWrite(_dirPin, HIGH);
      }
    }
    else{
      if(_dirCommand){
        digitalWrite(_dirPin, HIGH);
      }
      else{
        digitalWrite(_dirPin, LOW);
      }
    }      

    _steppingTimer.enable();
    _steppingTimer_stepOn.enable(); 
    
    _stepDelay = _degreePerStep / _rpm_speed / 360.0 * 60 * 1000; //ms, cast to int type
    // from RPM to °/ms --> (rpm * 360) / (60 * 1000)   [°/ms]
    // compute the ms you need to wait btw to degreePerStep steps: degreePerStep / stepDelay [ms]
    
    _actual_rpm_speed = _degreePerStep * 1000.0 / _stepDelay * 60 / 360;

    // intervallo minimo fra due step è 2ms (due fronti positivi del segnale allo step).
    // perché 1ms lo uso per tenere stabile il segnale alto.
    if(_stepDelay < 2){ 
      _stepDelay = 2;
    }
    _steppingTimer.setTimeToWait(_stepDelay);
  }
  else{
    _steppingTimer.disable();
    _steppingTimer_stepOn.disable();
  }

  // i timer girano
  _steppingTimer.periodicRun();
  _steppingTimer_stepOn.periodicRun();
  
  // genero lo step
  if(_stepCommand){
    if(_steppingInProgress){
      if(_steppingTimer_stepOn.getOutputTriggerEdgeType()){
        _steppingInProgress = false;
        digitalWrite(_stepPin, LOW);
      }
    }
    else{
      if(_steppingTimer.getOutputTriggerEdgeType()){
        _steppingTimer.reArm(); 
        _steppingTimer_stepOn.reArm(); 
        _steppingInProgress = true;
        digitalWrite(_stepPin, HIGH);
      }
    }
  }
  else{
	digitalWrite(_stepPin, LOW);
  }
}

void stepperMotor::stopMotor(void){
  _workingState = 0;
}

void stepperMotor::moveForward(float rpm_speed){
  _rpm_speed = rpm_speed;
  _workingState = 1;
}

void stepperMotor::moveBackward(float rpm_speed){
  _rpm_speed = rpm_speed;
  _workingState = 2;
}

void stepperMotor::switchPolarity(void){
  // chiami questo metodo per cambiare il senso di rotazione del motore, per compensare il montaggio.
  _switchPolarity = true;
}

float stepperMotor::get_actualRPMSpeed(void){
  return _actual_rpm_speed;
}


/* FILTERED INPUT - anti debounce */
antiDebounceInput::antiDebounceInput(byte pin, int debounceDelay){
  _inputPin = pin;  
  _changePolarity = false;
  _previousInputState = false;

  _lastDebounceTime = 0;
  _debounceDelay = debounceDelay;

  _inputState_output = false;
  }

void antiDebounceInput::periodicRun(void){
  _currentInputState = digitalRead(_inputPin);

  if(_currentInputState != _previousInputState){
    _lastDebounceTime = millis();
  }

  if((millis() - _lastDebounceTime) > _debounceDelay){
    if (_currentInputState != _inputState_output) {
      if(_changePolarity){
        _inputState_output = !(_currentInputState);
      }
      else{
        _inputState_output = _currentInputState;
      }
    }
  }
  
  _previousInputState = _currentInputState;
}

void antiDebounceInput::changePolarity(void){
  _changePolarity = true;
}

void antiDebounceInput::setDebounceDelay(int debounceDelay){
  _debounceDelay = debounceDelay;
}

bool antiDebounceInput::getInputState(void){
  return _inputState_output;
}
