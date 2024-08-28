#pragma once

void Ens160Aht2x_setup();
void Ens160Aht2x_loop();
void Ens160Aht2x_setDataListner(void func(float temp, float humidity, int aqi, int tvoc, int eco2));
float Ens160Aht2x_getTemperature();
float Ens160Aht2x_getHumidity();