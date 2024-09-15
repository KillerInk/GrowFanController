#pragma once

void Ens160Aht2x_setup();
void Ens160Aht2x_loop();
void Ens160Aht2x_setDataListner(void func(double temp, double humidity, int aqi, int tvoc, int eco2));
double Ens160Aht2x_getTemperature();
double Ens160Aht2x_getHumidity();
void Ens160Aht2x_setTempHumDif(double tempdif, double humdif);
double Ens160Aht2x_getTemperatureDif();
double Ens160Aht2x_getHumidityDif();
double Ens160Aht2x_getAvarageTemperature();
double Ens160Aht2x_getAvarageHumidity();
double Ens160Aht2x_getVpdAir();
double Ens160Aht2x_getVpdLeaf();
int Ens160Aht2x_getCo2();