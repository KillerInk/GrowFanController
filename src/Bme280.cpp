#include "Bme280.h"
#include "BME280I2C.h"
#include "MyMath.h"
#include "Wire.h"
#include "MyPreferences.h"
#include "config.h"

Bme280_data data;
const char *prefName = "Correction";
void (*bme280_eventlistner)(Bme280_data *data);

BME280I2C::Settings settings(
    BME280::OSR_X1,
    BME280::OSR_X1,
    BME280::OSR_X1,
    BME280::Mode_Forced,
    BME280::StandbyTime_1000ms,
    BME280::Filter_Off,
    BME280::SpiEnable_False,
#if BME_I2C_ADD == 119
    BME280I2C::I2CAddr_0x77
#else
    BME280I2C::I2CAddr_0x76
#endif // I2C address. I2C specific.
);

BME280I2C bme(settings);

void Bme280_setup()
{
    // data.temp_diff = MyPreferences_getDouble("Correction", "tempdif", data.temp_diff);
    // data.hum_diff = MyPreferences_getDouble("Correction", "humdif", data.hum_diff);

    Wire.begin();
    Wire.setClock(100000);
    if (!bme.begin())
    {
        log_e("Could not find a valid BME280 sensor, check wiring!");
    }
    switch (bme.chipModel())
    {
    case BME280::ChipModel_BME280:
        log_i("Found BME280 sensor! Success.");
        break;
    case BME280::ChipModel_BMP280:
        log_i("Found BMP280 sensor! No Humidity available.");
        break;
    default:
        log_i("Found UNKNOWN sensor! Error!");
    }
}

void Bme280_loop()
{

    bme.read(data.pressure, data.temperature, data.humidity, BME280::TempUnit_Celsius, BME280::PresUnit_hPa);
    data.avg_temperature = MyMath_avg(data.avg_temperature, data.temperature);
    data.avg_humidity = MyMath_avg(data.avg_humidity, data.humidity);
    data.avg_pressure = MyMath_avg(data.avg_pressure, data.pressure);
    data.vpdleaf = MyMath_vpd_leaf(data.avg_temperature, data.avg_humidity);
    // log_i("temp:%.2f a:%.2f humidity:%.2f a:%.2f pressure:%.2f a:%.2f vdp:%.2f", data.temperature, data.avg_temperature, data.humidity, data.avg_humidity, data.pressure, data.avg_pressure, data.vpdleaf);
    if (bme280_eventlistner != nullptr)
    {
        bme280_eventlistner(&data);
    }
}

void Bme280_setDataListner(void func(Bme280_data *data))
{
    bme280_eventlistner = func;
}

Bme280_data *Bme280_getData()
{
    return &data;
}

double Bme280_getTemperature()
{
    return data.temperature + data.temp_diff;
}

double Bme280_getHumidity()
{
    return data.humidity + data.hum_diff;
}

void Bme280_setTempHumDif(double tempdif, double humdif)
{
    data.temp_diff = tempdif;
    data.hum_diff = humdif;
    // MyPreferences_setDouble("Correction","tempdif", data.temp_diff);
    // MyPreferences_setDouble("Correction","humdif", data.hum_diff);
}

double Bme280_getTemperatureDif()
{
    return data.temp_diff;
}

double Bme280_getHumidityDif()
{
    return data.hum_diff;
}

double Bme280_getAvarageTemperature()
{
    return data.avg_temperature + data.temp_diff;
}

double Bme280_getAvarageHumidity()
{
    return data.avg_humidity + data.hum_diff;
}

double Bme280_getVpdAir()
{
    return 0.0;
}

double Bme280_getVpdLeaf()
{
    return data.vpdleaf;
}
