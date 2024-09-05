#pragma once
#include "Arduino.h"
#include "Voltage.h"

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
};

void FanController_setup();
void FanController_setHumidityAndTempFunctions(float func(), float func2());
void FanController_setAvgHumidityAndTempFunctions(float func(), float func2());
void FanController_applyspeed(int volt, int id, int val);
void FanController_processAutoControl();
void FanController_setVoltage(int id,int min, int max);
void FanController_setTargetTempHumSpeedDif(int temp, int hum,int speeddif);
void FanController_setAutoControl(bool enable);
void FanController_setMinMaxFanSpeed(int min, int max);
FanControllerValues * FanController_getValues();
Voltage * FanController_getFan0();
Voltage * FanController_getFan1();
