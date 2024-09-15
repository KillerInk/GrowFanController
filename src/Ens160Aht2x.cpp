#include "Ens160Aht2x.h"
#include <AHT20.h>
#include "DFRobot_ENS160.h"
#include "Preferences.h"

Preferences pref;
const char *prefName = "Correction";
DFRobot_ENS160_I2C ens160(&Wire, /*I2CAddr*/ 0x53);
AHT20 aht20;
double ens_temp = 0;
double ens_humidity = 0;
double temp_dif = 0.;
double hum_dif = 0.;
int AQI = 0;
int TVOC = 0; // ppb
int eCO2 = 0; // ppm
double svp = 0;
double avp = 0;
double vpd_leaf = 0;
double vpd_air = 0;
void (*ens_eventlistner)(double temp, double humidity, int aqi, int tvoc, int eco2);

double avarage_temp = 0.;
double avarage_humidity = 0.;

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
    temp_dif = pref.getDouble("tempdif", temp_dif);
    hum_dif = pref.getDouble("humdif", hum_dif);
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
    int t = avarage_temp * 100;
    avarage_temp = (double)t /100;
    t = avarage_humidity *100;
    avarage_humidity = (double)t / 100;
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
    int co2 = ens160.getECO2();
    if(eCO2 == 0)
        eCO2 = co2;
    eCO2 = 0.96 * eCO2 + 0.04 * co2;
    svp = 0.6108 * exp((17.67 * avarage_temp) / (avarage_temp + 243.5));
    avp = avarage_humidity / 100 * svp;
    vpd_leaf = svp -avp;
    vpd_air = (1-avarage_humidity/100) * svp;
    log_i("status:%i tvoc:%i eco2:%i aqi:%i svp %f avp %f vpd leaf %f vpd air %f", status, TVOC, eCO2, AQI, svp,avp, vpd_leaf, vpd_air);
    if (ens_eventlistner != nullptr)
        ens_eventlistner(Ens160Aht2x_getTemperature(), Ens160Aht2x_getHumidity(), AQI, TVOC, eCO2);
}

void Ens160Aht2x_setDataListner(void func(double temp, double humidity, int aqi, int tvoc, int eco2))
{
    ens_eventlistner = func;
}

double Ens160Aht2x_getTemperature()
{
    return ens_temp + temp_dif;
}

double Ens160Aht2x_getHumidity()
{
    return ens_humidity + hum_dif;
}

void Ens160Aht2x_setTempHumDif(double tempdif, double humdif)
{
    temp_dif = tempdif;
    hum_dif = humdif;
    pref.begin(prefName, false);
    pref.putDouble("tempdif", temp_dif);
    pref.putDouble("humdif", hum_dif);
    pref.end();
}

double Ens160Aht2x_getTemperatureDif()
{
    return temp_dif;
}

double Ens160Aht2x_getHumidityDif()
{
    return hum_dif;
}

double Ens160Aht2x_getAvarageTemperature()
{
    return avarage_temp + temp_dif;
}

double Ens160Aht2x_getAvarageHumidity()
{
    return avarage_humidity + hum_dif;
}

double Ens160Aht2x_getVpdAir()
{
    return vpd_air;
}

double Ens160Aht2x_getVpdLeaf()
{
    return vpd_leaf;
}

int Ens160Aht2x_getCo2()
{
    return eCO2;
}
