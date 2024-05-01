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
    void enable(void);
    void disable(void);
	void reset(void);

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
	
	bool _first_periodicRun;
	/* tiene traccia della prima chiamata a periodicRun. se passa molto tempo dall'inizializzazione al primo periodicRun rischio di triggerare un edge che non è 
	propriamente voluto. Gli edge si triggerano appena siamo in periodic run. */
};

/* STEPPER MOTOR CLASS */
class stepperMotor{
  public:
    stepperMotor(byte stepPin, byte dirPin, byte MS1, byte MS2, byte MS3, float degreePerStep);

    void periodicRun(void);

    void stopMotor(void);
    void moveForward(float rpm_speed);
    void moveBackward(float rpm_speed);
    void switchPolarity(void);


    //void setMicroSteppingConfiguration(bool MS1, bool MS2, bool MS3);
    float get_actualRPMSpeed(void);

  private:
    byte _stepPin;
    bool _stepCommand; // true = motore deve girare  false = motore fermo

    byte _dirPin;
    bool _dirCommand;
    bool _switchPolarity;

    byte _MS1, _MS2, _MS3; // microstepping configuration

    float _rpm_speed;
    float _degreePerStep;
    float _actual_rpm_speed;

    // stepping computation
    timer _steppingTimer; // timer che intervalla i fronti positivi
    timer _steppingTimer_stepOn; // timer che fa star su 1ms lo step
    int _stepDelay;

    bool _steppingInProgress;

    byte _workingState;
};

/* FILTERED INPUT - anti debounce */
class antiDebounceInput{
  public:
    antiDebounceInput(byte pin, int debounceTime);

    void periodicRun(void);
    void changePolarity(void);

    void setDebounceDelay(int debounceDelay);
    bool getInputState(void); // ritorna dicendo se il pulsante è premuto o no

  private:
    byte _inputPin;
    bool _changePolarity;

    bool _currentInputState;
    bool _previousInputState;

    unsigned long _lastDebounceTime; 
    unsigned long _debounceDelay; 

    bool _inputState_output; // variabile di output di questo oggetto che mi dice lo stato dell'input    
};
#endif
