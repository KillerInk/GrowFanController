#pragma once

void GoveeBTh5179_setup();
void GoveeBTh5179_loop();
float GoveeBTh5179_getTemperature();
float GoveeBTh5179_getHumidity();
int GoveeBTh5179_getBattery();
void GoveeBTh5179_setEventListner(void func(float temp, float hum, int bat));
void GoveeBTh5179_enable(bool enable);
bool GoveeBTh5179_isEnable();
