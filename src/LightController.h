#pragma once
#include "MyTime.h"
#include "Voltage.h"

enum light_state
{
    off,
    on,
    sunrise,
    sunset,
};

struct LightControllerValues
{
    Voltage voltage;
    MyTime turnOnTime;
    MyTime turnOffTime;
    MyTime sunriseEnd;
    MyTime sunsetStart;
    int minLightP = 20;
    int maxLightP = 80;
    int currentLightP;

    bool enableSunrise = false;
    bool enableSunset = false;

    light_state current_state = off;
    bool automode = false;
};

void LightController_setup();
void LightController_loop();
void LightController_setVoltageLimits(int min, int max);
void LightController_setPercentLimits(int min, int max);
void LightController_setTimes(int onhour, int onmin, int offhour, int offmin, int risehour, int risemin, int sethour, int setmin,bool riseenable, bool setenable);
void LightController_setLight(int mv);
void LightController_setAutoMode(bool active);
LightControllerValues * LightController_getValues();