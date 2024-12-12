#pragma once

struct Bme280_data
{
    float temperature, avg_temperature, humidity, avg_humidity, pressure, avg_pressure, hum_diff,temp_diff, vpdleaf;
};

void Bme280_setup();
void Bme280_loop();
void Bme280_setDataListner(void func(Bme280_data * data));
Bme280_data * Bme280_getData();

double Bme280_getTemperature();
double Bme280_getHumidity();
void Bme280_setTempHumDif(double tempdif, double humdif);
double Bme280_getTemperatureDif();
double Bme280_getHumidityDif();
double Bme280_getAvarageTemperature();
double Bme280_getAvarageHumidity();
double Bme280_getVpdAir();
double Bme280_getVpdLeaf();