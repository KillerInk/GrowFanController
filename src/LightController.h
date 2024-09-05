#pragma once

void LightController_setup();
void LightController_loop();
void LightController_setVoltageLimits(int min,int max);
void LightController_setTimes(int onhour, int onmin,int offhour,int offmin,int risehour, int risemin, int sethour, int setmin);
void LightController_setLight(int mv);