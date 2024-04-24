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
}

void timer::periodicRun(void){
  if(_enable){
    _outputTrigger_edgeType = false;


    if(!_outputTrigger_stableType){
      /*
        Fintanto che non ho dato il trigger di output faccio contare il timer. Appena l'ho dato, ho bisogno che ci sia un qualcuno che esternamente
        ri-armi il trigger così che possa tornare a contare. Dall'esterno qualcuno mette reArm = true

        Ricorda che ci sono le due uscite del timer. Una che sta su finché non riparto a contare, una che funziona ad edge e sta su per un ciclo PLC.
      */
      _currentTime = millis();

      
      _outputTrigger_stableType = false;

      if(_resetTimer){
        _lastTriggerTime = _currentTime;
        _resetTimer = false;
      }

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
} 

void timer::setTimeToWait(int timeToWait){
  _timeToWait = timeToWait;
}

bool timer::getOutputTriggerEdgeType(void){
  return _outputTrigger_edgeType;
}

bool timer::getOutputTriggerStableType(void){
  return _outputTrigger_stableType;
}
