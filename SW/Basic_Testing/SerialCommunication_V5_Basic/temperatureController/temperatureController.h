#ifndef temperatureController_h
#define temperatureController_h
#include "Arduino.h"

#define DEFAULT_HYSTERESIS_RANGE 2.5
#define DEFAULT_REFERENCE_TEMPERATURE 37.5

class temperatureController{
  public:
    temperatureController();

    void periodicRun(float *temperatures, byte dimension);
    void setTemperatureHysteresis(float lowerTemperature, float higherTemperature);
    void setControlModality(int controlModality);

    bool getOutputState(void);
    
    // debug
    float debug_getActualTemperature(void);
    int debug_getHysteresisState(void);

  private:
    // hysteresis parameters
    float _referenceTemperature;
    float _higherTemperature;
    float _lowerTemperature;

    float _actualTemperature;

    bool _outputState;

    int _controlModality;
    /*
      0: free
      1: controllo basato sulla temperatura più alta
      2: controllo basato sulla media
    */

    int _hysteresisState;


};
#endif
