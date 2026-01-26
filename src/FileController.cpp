#include "FileController.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "time.h"
#ifdef SENSOR_BME280
#include "Bme280.h"
#endif
#ifdef SENSOR_ENS160AHT21
#include "Ens160Aht2x.h"
#endif
#include "FanController.h"
#include "LightController.h"

char filename[] = "/yyyymmdd.csv";
bool havesdcard = false;
bool sdinit = false;
xSemaphoreHandle xMutex = xSemaphoreCreateMutex();

bool haveSdInsert()
{
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        log_i("No SD card attached");
        return false;
    }
    if (cardType == CARD_MMC)
        log_i("SD Card Type: MMC");
    else if (cardType == CARD_SD)
        log_i("SD Card Type: SDSC");
    else if (cardType == CARD_SDHC)
        log_i("SD Card Type: SDHC");
    else
        log_i("SD Card Type: UNKNOWN");
    return true;
}

void FileController_setup()
{
    int ret = 0;
    while (!SD.begin(5, SPI, 40000000UL) && ret < 5)
    {
        log_i("Card Mount Failed");
        vTaskDelay(50);
        ret++;
    }
    if (ret == 5)
        return;
    sdinit = true;

    havesdcard = haveSdInsert();
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    uint64_t usedbytes = SD.usedBytes() / (1024 * 1024);
    log_i("SD Card Size: %lluMB  used %lluMB", cardSize, usedbytes);
}

void FileController_write(double temp, double hum, int fanspeed, int co2, int lightmv, double vpd)
{
    if (xSemaphoreTake(xMutex, portMAX_DELAY))
    {
        if (!sdinit)
            return;
        if (!havesdcard)
        {
            havesdcard = haveSdInsert();
            log_i("checked if sd is insert:%i", havesdcard);
            if (!havesdcard)
                return;
        }
        tm time;
        getLocalTime(&time);
        if (time.tm_isdst)
            time.tm_hour++;
        int year = time.tm_year + 1900;
        String ret = "/" + String(year);
        if (!SD.exists(ret))
            SD.mkdir(ret);
        int month = time.tm_mon + 1;
        if (month < 10)
            ret = ret + "/" + "0" + String(month);
        else
            ret = ret + "/" + String(month);
        if (!SD.exists(ret))
            SD.mkdir(ret);
        int day = time.tm_mday;
        if (day < 10)
            ret = ret + "/" + "0" + String(day);
        else
            ret = ret + "/" + String(day);
        if (!SD.exists(ret))
            SD.mkdir(ret);
        if (time.tm_hour < 10)
            ret = ret + "/" + "0" + String(time.tm_hour) + ".csv";
        else
            ret = ret + "/" + String(time.tm_hour) + ".csv";
        File myFile;
        if (!SD.exists(ret))
        {
            log_i("create new file %s", ret.c_str());
            myFile = SD.open(ret, FILE_WRITE);
        }
        else
        {
            // log_i("append to file %s", ret.c_str());
            myFile = SD.open(ret, FILE_APPEND);
        }
        if (myFile)
        {
            myFile.printf("%i:%i:%i, ", time.tm_hour, time.tm_min, time.tm_sec);
            myFile.printf("%.2f, ", temp);
            myFile.printf("%.2f, ", hum);
            myFile.printf("%i, ", fanspeed);
            myFile.printf("%i, ", co2);
            myFile.printf("%i, ", lightmv);
            myFile.printf("%.3f\r\n", vpd);
            myFile.close();
        }
        else
        {
            log_e("Failed to write to file %s", ret.c_str());
            havesdcard = false;
        }
        xSemaphoreGive(xMutex);
    }
}

String FileController_read(String name)
{
    String ret;
    if (xSemaphoreTake(xMutex, portMAX_DELAY))
    {
        File myFile = SD.open(name, FILE_READ);
        ret = myFile.readString();
        myFile.close();
        xSemaphoreGive(xMutex);
    }
    return ret;
}

/* --------------------------------------------------------------------------- */
/* NEW: write all sensor data in the same format as sendSocketMsg()            */
/* --------------------------------------------------------------------------- */
void FileController_write()
{
    if (xSemaphoreTake(xMutex, portMAX_DELAY))
    {
        if (!sdinit)
        {
            xSemaphoreGive(xMutex);
            return;
        }
        if (!havesdcard)
        {
            havesdcard = haveSdInsert();
            log_i("checked if sd is insert:%i", havesdcard);
            if (!havesdcard)
            {
                xSemaphoreGive(xMutex);
                return;
            }
        }

        tm time;
        getLocalTime(&time);
        if (time.tm_isdst)
            time.tm_hour++;

        /* build directory path: /YYYY/MM/DD/HH.csv */
        String ret = "/" + String(time.tm_year + 1900);
        if (!SD.exists(ret))
            SD.mkdir(ret);
        int month = time.tm_mon + 1;
        if (month < 10)
            ret = ret + "/" + "0" + String(month);
        else
            ret = ret + "/" + String(month);
        if (!SD.exists(ret))
            SD.mkdir(ret);
        int day = time.tm_mday;
        if (day < 10)
            ret = ret + "/" + "0" + String(day);
        else
            ret = ret + "/" + String(day);
        if (!SD.exists(ret))
            SD.mkdir(ret);
        if (time.tm_hour < 10)
            ret = ret + "/" + "0" + String(time.tm_hour) + ".csv";
        else
            ret = ret + "/" + String(time.tm_hour) + ".csv";

        File myFile;
        bool isNew = !SD.exists(ret); // check before opening
        if (isNew)
            myFile = SD.open(ret, FILE_WRITE);
        else
            myFile = SD.open(ret, FILE_APPEND);
        if (!myFile)
        {
            log_e("Failed to open file %s", ret.c_str());
            havesdcard = false;
            xSemaphoreGive(xMutex);
            return;
        }

        if (isNew)
        {
            myFile.printf("time,");

#ifdef SENSOR_BME280
            myFile.printf("tempB,humB,avgTempB,avgHumB,presB,avgPresB,vpdAirB,");
#endif
#ifdef SENSOR_ENS160AHT21
            myFile.printf("tempE,humE,avgTempE,avgHumE,eco2,aqi,tvoc,vpdAirE,");
#endif
            myFile.printf("volt0,volt1,lightP,lightMv\r\n");
        }

        /* ---- sensor values ------------------------------------------------- */
        /* BME280 data (if available) */
#ifdef SENSOR_BME280
        double tempB = Bme280_getTemperature();
        double humB = Bme280_getHumidity();
        double avgTempB = Bme280_getAvarageTemperature();
        double avgHumB = Bme280_getAvarageHumidity();
        double presB = Bme280_getData()->pressure;
        double avgPresB = Bme280_getData()->avg_pressure;
        double vpdAirB = Bme280_getVpdLeaf();
#endif

        /* ENS160/AHT21 data (if available) */
#ifdef SENSOR_ENS160AHT21
        double tempE = Ens160Aht2x_getTemperature();
        double humE = Ens160Aht2x_getHumidity();
        double avgTempE = Ens160Aht2x_getAvarageTemperature();
        double avgHumE = Ens160Aht2x_getAvarageHumidity();
        int eco2 = Ens160Aht2x_getCo2();
        int aqi = Ens160Aht2x_getAqi();
        int tvoc = Ens160Aht2x_getTvoc();
        double vpdAirE = Ens160Aht2x_getVpdLeaf();
#endif

        /* Fan controller values */
        double volt0 = FanController_getFan0()->voltage;
        double volt1 = FanController_getFan1()->voltage;

        /* Light controller values */
        double lightP = LightController_getValues()->currentLightP;
        double lightMv = LightController_getValues()->voltage.voltage;
        int lightState = LightController_getValues()->current_state;

        /* VPD (if available) */
#ifdef SENSOR_BME280

#else
        double vpdAir = 0.0;
#endif

        /* --------------------------------------------------------------------- */
        time_t epoch = mktime(&time); 
        myFile.printf("%ld,", epoch);
#ifdef SENSOR_BME280
        /* BME280 */
        myFile.printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.3f,",
                      tempB, humB, avgTempB, avgHumB, presB, avgPresB, vpdAirB);
#endif

/* ENS160/AHT21 */
#ifdef SENSOR_ENS160AHT21
        myFile.printf("%.2f,%.2f,%.2f,%.2f,%i,%i,%i,%.3f,",
                      tempE, humE, avgTempE, avgHumE, eco2, aqi, tvoc, vpdAirE);
#endif

        /* Fan */
        myFile.printf("%.2f,%.2f,", volt0, volt1);

        /* Light */
        myFile.printf("%.2f,%.2f\r\n", lightP, lightMv);

        myFile.close();
        xSemaphoreGive(xMutex);
    }
}
