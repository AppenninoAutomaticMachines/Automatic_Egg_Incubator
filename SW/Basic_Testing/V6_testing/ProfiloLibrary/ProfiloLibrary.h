#ifndef ProfiloLibrary_h
#define ProfiloLibrary_h
#include "Arduino.h"

#define DEFAULT_HYSTERESIS_RANGE 2.5
#define DEFAULT_REFERENCE_TEMPERATURE 37.5

/* TEMPERATURE CONTROLLER CLASS */
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

/* TIMER CLASS */
class timer{
  public:
    timer();

    void periodicRun(void);
    void reArm(void);

    void setTimeToWait(int timeToWait);
    
    bool getOutputTriggerEdgeType(void);
    bool getOutputTriggerStableType(void);


  private:
    unsigned long _currentTime, _lastTriggerTime;

    int _timeToWait;

    bool _enable;
    bool _outputTrigger_edgeType;
    bool _outputTrigger_stableType;

    bool _resetTimer; // per ripartire a contare
    bool _reArm; // funziona a trigger
};
#endif
