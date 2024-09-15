#pragma once

void Ens160Aht2x_setup();
void Ens160Aht2x_loop();
void Ens160Aht2x_setDataListner(void func(float temp, float humidity, int aqi, int tvoc, int eco2));
float Ens160Aht2x_getTemperature();
float Ens160Aht2x_getHumidity();
void Ens160Aht2x_setTempHumDif(float tempdif, float humdif);
float Ens160Aht2x_getTemperatureDif();
float Ens160Aht2x_getHumidityDif();
float Ens160Aht2x_getAvarageTemperature();
float Ens160Aht2x_getAvarageHumidity();
float Ens160Aht2x_getVpdAir();
float Ens160Aht2x_getVpdLeaf();
int Ens160Aht2x_getCo2();