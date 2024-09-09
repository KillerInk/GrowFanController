#pragma once
#include "Arduino.h"
#include "Voltage.h"
#include "MyTime.h"

struct FanControllerValues
{
    int targetTemperature = 26;
    int targetHumidity = 60;
    bool autocontrol = false;
    // tracked fanspeed in autocontrol in %
    int autocontrolfanspeed = 50; // %
    int minspeed = 0;
    int maxspeed = 100;
    // in blowing fans need to run slower to compensate the resistance from the filter and keep a bit vacuum inside the tent
    int filtercompensation = 5; //%

    bool nightmode = false;
    bool nightmodeActive = false;
    int nightmodeMaxSpeed;
    MyTime nightmodeOn;
    MyTime nightModeOff;
    // mars hydro fan 560-630
    // artic 120mm 560-1100;
    Voltage fan0Voltage;
    Voltage fan1Voltage;
};

void FanController_setup();
void FanController_setHumidityAndTempFunctions(float func(), float func2());
void FanController_setAvgHumidityAndTempFunctions(float func(), float func2());
void FanController_applyspeed(int volt, int id, int val);
void FanController_processAutoControl();
void FanController_setVoltage(int id, int min, int max);
void FanController_setTargetTempHumSpeedDif(int temp, int hum, int speeddif);
void FanController_setAutoControl(bool enable);
void FanController_setMinMaxFanSpeed(int min, int max);
FanControllerValues *FanController_getValues();
Voltage *FanController_getFan0();
Voltage *FanController_getFan1();
void FanController_loop();

void FanController_setNightModeValues(int onhour, int onmin, int offhour, int offmin, int maxspeed);
void FanController_setNightMode(bool active);
