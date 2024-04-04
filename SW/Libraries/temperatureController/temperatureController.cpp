#include "Arduino.h"
#include "temperatureController.h"

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

  switch(_controlModality){
    case 0:
      break;

    case 1:
      float maxTemperature = temperatures[0];
      for (int i = 0; i < dimension; i++) {
        if (temperatures[i] > maxTemperature) {
          maxTemperature = temperatures[i];
        }
      }
      _actualTemperature = maxTemperature;

      break;

    case 2:
      float meanTemperature;
      float sum = 0.0;
      for (int i = 0; i < dimension; i++) {
        sum = sum + temperatures[i];
      }
      _actualTemperature = sum / dimension;

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

void temperatureController::setControlModality(byte controlModality){
  _controlModality = controlModality;
}

bool temperatureController::getOutputState(void){
  return _outputState;
}

float temperatureController::debug_getActualTemperature(void){
  return _actualTemperature;
}

byte temperatureController::debug_getHysteresisState(void){
  return _hysteresisState;
}






