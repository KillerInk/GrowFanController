#include "Ens160Aht2x.h"
#include <AHT20.h>
#include "DFRobot_ENS160.h"
#include "Preferences.h"

Preferences pref;
const char *prefName = "Correction";
DFRobot_ENS160_I2C ens160(&Wire, /*I2CAddr*/ 0x53);
AHT20 aht20;
float ens_temp = 0;
float ens_humidity = 0;
float temp_dif = 0.;
float hum_dif = 0.;
int AQI = 0;
int TVOC = 0; // ppb
int eCO2 = 0; // ppm
void (*ens_eventlistner)(float temp, float humidity, int aqi, int tvoc, int eco2);

float avarage_temp = 0.;
float avarage_humidity = 0.;

uint8_t checkI2C(uint8_t addr)
{
    Wire.beginTransmission(addr);
    byte error;
    error = Wire.endTransmission();
    if (error == 0)
        return 1;
    return 0;
}

void Ens160Aht2x_setup()
{
    // sda/scl pin 21/22
    Wire.begin();
    Wire.setClock(100000);
    pref.begin(prefName, false);
    temp_dif = pref.getFloat("tempdif", temp_dif);
    hum_dif = pref.getFloat("humdif", hum_dif);
    pref.end();

    for (byte i = 1; i < 127; i++)
    {
        int avail = checkI2C(i);
        if (avail == 1)
            log_i("found address %i", i);
    }
    // Wire.end();
    boolean rdy = 0;
    rdy = aht20.begin();
    log_i("aht avail:%i", rdy);

    // log_i("firmware: %s",ens160.getFirmwareVersion());
    ens160.setPWRMode(ENS160_STANDARD_MODE);
}

void Ens160Aht2x_loop()
{

    ens_temp = aht20.getTemperature();
    ens_humidity = aht20.getHumidity();
    if (avarage_temp == 0.)
        avarage_temp = ens_temp;
    if (avarage_humidity == 0.)
        avarage_humidity = ens_humidity;

    avarage_temp = 0.96 * avarage_temp + 0.04 * ens_temp;
    avarage_humidity = 0.96 * avarage_humidity + 0.04 * ens_humidity;
    log_i("temp:%f a: %f humidity:%f a:%f ", ens_temp, avarage_temp, ens_humidity, avarage_humidity);
    ens160.setTempAndHum(Ens160Aht2x_getTemperature(), Ens160Aht2x_getHumidity());
    /*
     *         1-Warm-Up phase, first 3 minutes after power-on.
     *         2-Initial Start-Up phase, first full hour of operation after initial power-on. Only once in the sensor’s lifetime.
     */
    int status = ens160.getENS160Status();
    // Return value range: 1-5 (Corresponding to five levels of Excellent, Good, Moderate, Poor and Unhealthy respectively)
    AQI = (uint8_t)ens160.getAQI();
    TVOC = ens160.getTVOC();
    eCO2 = ens160.getECO2();
    log_i("status:%i tvoc:%i eco2:%i aqi:%i", status, TVOC, eCO2, AQI);
    if (ens_eventlistner != nullptr)
        ens_eventlistner(Ens160Aht2x_getTemperature(), Ens160Aht2x_getHumidity(), AQI, TVOC, eCO2);
}

void Ens160Aht2x_setDataListner(void func(float temp, float humidity, int aqi, int tvoc, int eco2))
{
    ens_eventlistner = func;
}

float Ens160Aht2x_getTemperature()
{
    return ens_temp + temp_dif;
}

float Ens160Aht2x_getHumidity()
{
    return ens_humidity + hum_dif;
}

void Ens160Aht2x_setTempHumDif(float tempdif, float humdif)
{
    temp_dif = tempdif;
    hum_dif = humdif;
    pref.begin(prefName, false);
    pref.putFloat("tempdif", temp_dif);
    pref.putFloat("humdif", hum_dif);
    pref.end();
}

float Ens160Aht2x_getTemperatureDif()
{
    return temp_dif;
}

float Ens160Aht2x_getHumidityDif()
{
    return hum_dif;
}

float Ens160Aht2x_getAvarageTemperature()
{
    return avarage_temp + temp_dif;
}

float Ens160Aht2x_getAvarageHumidity()
{
    return avarage_humidity + hum_dif;
}
