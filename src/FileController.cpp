#include "FileController.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "time.h"

File myFile;
char filename[] = "/yyyymmdd.csv";
bool havesdcard = false;

void FileController_setup()
{
    if (!SD.begin(5))
    {
        log_i("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        log_i("No SD card attached");
        return;
    }
    havesdcard = true;
    if (cardType == CARD_MMC)
        log_i("SD Card Type: MMC");
    else if (cardType == CARD_SD)
        log_i("SD Card Type: SDSC");
    else if (cardType == CARD_SDHC)
        log_i("SD Card Type: SDHC");
    else
        log_i("SD Card Type: UNKNOWN");
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    log_i("SD Card Size: %lluMB\n", cardSize);
}

int currentday = -1;
void FileController_write(float temp, float hum, int fanspeed, int co2, int lightmv, float vpd)
{
    if (!havesdcard)
        return;
    tm time;
    getLocalTime(&time);
    int year = time.tm_year;
    int month = time.tm_mon;
    int day = time.tm_mday;
    if (currentday != day)
    {
        log_i("new day, new file?");
        sprintf(filename, "/%0004i%02i%02i.csv", year + 1900, month + 1, day);
        currentday = day;
    }
    if (!SD.exists(filename))
    {
        log_i("create new file %s", filename);
        myFile = SD.open(filename, "w");
    }
    else
    {
        log_i("append to file %s", filename);
        myFile = SD.open(filename, "a");
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
        /*if (firstitem)
        {
            myFile.print("[{");
            firstitem = false;
        }
        else
        {
            myFile.seek(myFile.size() - 1);
            myFile.print(",{");
        }
        myFile.printf("\"time\":\"%i:%i:%i\",", time.tm_hour, time.tm_min, time.tm_sec);
        myFile.printf("\"temp\":%.2f,", temp);
        myFile.printf("\"hum\":%.2f,", hum);
        myFile.printf("\"fanspeed\":%i,", fanspeed);
        myFile.printf("\"co2\":%i,", co2);
        myFile.printf("\"lightmv\":%i,", lightmv);
        myFile.printf("\"vpd\":%.2f", vpd);
        myFile.print("}]");

        */
        myFile.close();
    }
    else
    {
        log_e("Failed to write to file %s", filename);
        havesdcard = false;
    }
}
