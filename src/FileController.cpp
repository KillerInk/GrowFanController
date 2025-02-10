#include "FileController.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "time.h"

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
    int ret =0;
    while (!SD.begin(5,SPI,40000000UL) && ret < 5)
    {
        log_i("Card Mount Failed");
        vTaskDelay(50);
        ret++;
    }
    if(ret == 5)
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
