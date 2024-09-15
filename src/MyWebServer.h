#pragma once
#include "Arduino.h"

struct MyWebServerMethodCallbacks
{
    void (*applyspeed_listner)(int volt, int id, int val);
    void (*voltagechanged_listner)(int id, int min, int max);
    void (*targettemphum_listner)(int tmp, int hum, int speed);
    void (*autocontrol_listner)(bool enable);
    String (*getFanControllerSettings)();
    void (*readgovee_listner)(bool enable);
    void (*setTempHumDif)(double temp, double hum);
    void (*setMinMaxSpeed)(int min, int max);
    void (*fancoltroller_nightmodecallback)(int onhour, int onmin, int offhour, int offmin, int maxspeed);
    void (*fancoltroller_nightmodeactivcecallback)(bool active);
    void (*lightController_setVoltageLimits)(int min, int max);
    void (*lightController_setTimes)(int onhour, int onmin, int offhour, int offmin, int risehour, int risemin, int sethour, int setmin,bool riseenable, bool setenable);
    void (*lightController_setLight)(int mv);
    void (*lightController_setAuto)(bool automode);
};

MyWebServerMethodCallbacks * MyWebServer_getCallbacksStruct();
void MyWebServer_setup();
void MyWebServer_sendSocketMsg(String msg);