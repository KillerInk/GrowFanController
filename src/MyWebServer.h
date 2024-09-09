#pragma once
#include "Arduino.h"

void MyWebServer_setup();
void MyWebServer_sendSocketMsg(String msg);
void MyWebServer_setApplySpeedListner(void func(int volt,int id,int val));
void MyWebServer_setVoltageChangedListner(void func(int id,int min,int max));
void MyWebServer_setTargetTempHumChangedListner(void func(int tmp,int hum,int speed));
void MyWebServer_setAutoControlListner(void func(bool enable));
void MyWebServer_setFanControllerGetSettings(String func());
void MyWebServer_setReadGoveeListner(void func(bool enable));
void MyWebServer_setTempHumDif(void func(float temp, float hum));
void MyWebServer_setMinMaxSpeed(void func(int min,int max));
void MyWebServer_setFanControllerNightModeCallback(void func(int onhour, int onmin, int offhour, int offmin,int maxspeed));
void MyWebServer_setFanControllerNightModeActiveCallback(void func(bool active));