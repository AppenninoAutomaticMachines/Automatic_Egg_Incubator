#include "Arduino.h"
#include "temperatureController.h"

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
