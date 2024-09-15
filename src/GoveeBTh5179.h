#pragma once

void GoveeBTh5179_setup();
void GoveeBTh5179_loop();
double GoveeBTh5179_getTemperature();
double GoveeBTh5179_getHumidity();
int GoveeBTh5179_getBattery();
void GoveeBTh5179_setEventListner(void func(double temp, double hum, int bat));
void GoveeBTh5179_enable(bool enable);
bool GoveeBTh5179_isEnable();
