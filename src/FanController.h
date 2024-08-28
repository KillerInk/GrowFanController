#pragma once
#include "Arduino.h"

void FanController_setup();
void FanController_setHumidityAndTempFunctions(float func(), float func2());
void FanController_applyspeed(int volt, int id, int val);
void FanController_processAutoControl();
bool FanController_isAutoControl();
int FanController_getAutoSpeed();
void FanController_setVoltage(int id,int min, int max);
void FanController_setTargetTempHum(int temp, int hum);
void FanController_setAutoControl(bool enable);
String FanController_getSettings();