#pragma once
#include "Arduino.h"

void MyWebServer_setup();
void MyWebServer_sendSocketMsg(String msg);
void MyWebServer_setApplySpeedListner(void func(int volt,int id,int val));
void MyWebServer_setVoltageChangedListner(void func(int id,int min,int max));
void MyWebServer_setTargetTempHumChangedListner(void func(int tmp,int hum));
void MyWebServer_setAutoControlListner(void func(bool enable));
void MyWebServer_setFanControllerGetSettings(String func());