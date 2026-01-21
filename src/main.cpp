#include <Arduino.h>
#include <FS.h>
#include <WiFi.h>

#include "mdns.h"
#include "FanController.h"
#include "Arduino_JSON.h"
#ifdef GOVEE_BTH5179
#include "GoveeBTh5179.h"
#endif
#ifdef SENSOR_ENS160AHT21
#include "Ens160Aht2x.h"
#endif
#include "MyWebServer.h"
#include "time.h"
#include "LightController.h"
#ifdef USE_SDCARD
#include "FileController.h"
#endif
#ifdef SENSOR_BME280
#include "Bme280.h"
#endif
#include <nvs_flash.h>

#ifdef GOVEE_BTH5179
typedef struct
{
    double temp, hum, bat;
} govee_data;

govee_data h5179_data{.temp = 0, .hum = 0, .bat = 0};

void govee_dataListner(double temp, double hum, int bat)
{
    h5179_data.temp = temp;
    h5179_data.hum = hum;
    h5179_data.bat = bat;
}
#endif

void sendSocketMsg()
{
    JSONVar socketmsg;
    char buf[64];
    int ret;
#ifdef GOVEE_BTH5179
    if (h5179_data.temp > 0 && h5179_data.hum > 0 && h5179_data.bat > 0)
    {
        snprintf(buf, sizeof buf, "%.2f", h5179_data.temp);
        socketmsg["govee"]["temperatur"] = buf;
        snprintf(buf, sizeof buf, "%.2f", h5179_data.hum);
        socketmsg["govee"]["humidity"] = buf;
        socketmsg["govee"]["battery"] = h5179_data.bat;
    }
#endif
#ifdef SENSOR_BME280
    ret = snprintf(buf, sizeof buf, "%.2f", Bme280_getTemperature());
    socketmsg["bme280"]["temperatur"] = buf;
    ret = snprintf(buf, sizeof buf, "%.2f", Bme280_getHumidity());
    socketmsg["bme280"]["humidity"] = buf;
    ret = snprintf(buf, sizeof buf, "%.2f", Bme280_getAvarageTemperature());
    socketmsg["bme280"]["atemperatur"] = buf;
    ret = snprintf(buf, sizeof buf, "%.2f", Bme280_getAvarageHumidity());
    socketmsg["bme280"]["ahumidity"] = buf;
    ret = snprintf(buf, sizeof buf, "%.2f", Bme280_getData()->pressure);
    socketmsg["bme280"]["pressure"] = buf;
    ret = snprintf(buf, sizeof buf, "%.2f", Bme280_getData()->avg_pressure);
    socketmsg["bme280"]["apressure"] = buf;
#endif

#ifdef SENSOR_ENS160AHT21
    ret = snprintf(buf, sizeof buf, "%.2f", Ens160Aht2x_getTemperature());
    socketmsg["ens160aht21"]["temperatur"] = buf;
    ret = snprintf(buf, sizeof buf, "%.2f", Ens160Aht2x_getHumidity());
    socketmsg["ens160aht21"]["humidity"] = buf;
    ret = snprintf(buf, sizeof buf, "%.2f", (Ens160Aht2x_getAvarageTemperature()));
    socketmsg["ens160aht21"]["atemperatur"] = buf;
    ret = snprintf(buf, sizeof buf, "%.2f", (Ens160Aht2x_getAvarageHumidity()));
    socketmsg["ens160aht21"]["ahumidity"] = buf;
    socketmsg["ens160aht21"]["eco2"] = Ens160Aht2x_getCo2();
    socketmsg["ens160aht21"]["aqi"] = Ens160Aht2x_getAqi();
    socketmsg["ens160aht21"]["tvoc"] = Ens160Aht2x_getTvoc();
#endif

    if (FanController_getValues()->autocontrol)
    {
        socketmsg["autocontrolspeed"] = FanController_getValues()->autocontrolfanspeed;
    }
    socketmsg["voltage0"] = FanController_getFan0()->voltage;
    socketmsg["voltage1"] = FanController_getFan1()->voltage;
    socketmsg["nightmode"] = FanController_getValues()->nightmodeActive;
    tm time;
    getLocalTime(&time);
    if (time.tm_isdst)
        time.tm_hour++;
    ret = snprintf(buf, sizeof buf, "%02i:%02i:%02i", time.tm_hour, time.tm_min, time.tm_sec);
    socketmsg["time"] = buf;
    socketmsg["lightvalP"] = LightController_getValues()->currentLightP;
    socketmsg["lightvalmv"] = LightController_getValues()->voltage.voltage;
    socketmsg["lightstate"] = LightController_getValues()->current_state;
#ifdef SENSOR_BME280
    ret = snprintf(buf, sizeof buf, "%.2f", Bme280_getVpdLeaf());
    socketmsg["vpdair"] = buf;
#endif
    MyWebServer_sendSocketMsg(JSON.stringify(socketmsg));
}

String getSettings()
{
    JSONVar myObject;
    myObject["fan0voltage"] = FanController_getFan0()->voltage;
    myObject["fan0min"] = FanController_getFan0()->min;
    myObject["fan0max"] = FanController_getFan0()->max;
    myObject["fan1voltage"] = FanController_getFan1()->voltage;
    myObject["fan1min"] = FanController_getFan1()->min;
    myObject["fan1max"] = FanController_getFan1()->max;
    myObject["autocontrol"] = FanController_getValues()->autocontrol;
    myObject["targetTemperature"] = FanController_getValues()->targetTemperature;
    myObject["targetHumidity"] = FanController_getValues()->targetHumidity;
#ifdef GOVEE_BTH5179
    myObject["readgovee"] = GoveeBTh5179_isEnable();
#endif
    myObject["speeddif"] = FanController_getValues()->filtercompensation;
#ifdef SENSOR_ENS160AHT21
    myObject["tempdif"] = Ens160Aht2x_getTemperatureDif();
    myObject["humdif"] = Ens160Aht2x_getHumidityDif();
#endif
#ifdef SENSOR_BME280
#ifndef SENSOR_ENS160AHT21
    myObject["tempdif"] = Bme280_getTemperatureDif();
    myObject["humdif"] = Bme280_getHumidityDif();
#endif
#endif
    myObject["minspeed"] = FanController_getValues()->minspeed;
    myObject["maxspeed"] = FanController_getValues()->maxspeed;

    myObject["nightmodeactive"] = FanController_getValues()->nightmode;
    myObject["nightmodeonhour"] = FanController_getValues()->nightmodeOn.hour;
    myObject["nightmodeonmin"] = FanController_getValues()->nightmodeOn.min;
    myObject["nightmodeoffmin"] = FanController_getValues()->nightModeOff.min;
    myObject["nightmodeoffhour"] = FanController_getValues()->nightModeOff.hour;
    myObject["nightmodemaxspeed"] = FanController_getValues()->nightmodeMaxSpeed;

    myObject["lightonh"] = LightController_getValues()->turnOnTime.hour;
    myObject["lightonmin"] = LightController_getValues()->turnOnTime.min;
    myObject["lightoffh"] = LightController_getValues()->turnOffTime.hour;
    myObject["lightoffmin"] = LightController_getValues()->turnOffTime.min;
    myObject["lightriseh"] = LightController_getValues()->sunriseEnd.hour;
    myObject["lightrisemin"] = LightController_getValues()->sunriseEnd.min;
    myObject["lightseth"] = LightController_getValues()->sunsetStart.hour;
    myObject["lightsetmin"] = LightController_getValues()->sunsetStart.min;
    myObject["lightriseenable"] = LightController_getValues()->enableSunrise;
    myObject["lightsetenable"] = LightController_getValues()->enableSunset;
    myObject["lightautomode"] = LightController_getValues()->automode;
    myObject["lightminvolt"] = LightController_getValues()->voltage.min;
    myObject["lightmaxvolt"] = LightController_getValues()->voltage.max;
    myObject["lightlimitspmin"] = LightController_getValues()->minLightP;
    myObject["lightlimitspmax"] = LightController_getValues()->maxLightP;

    myObject["cloud"]["active"] = LightController_getValues()->cloudsim;
    myObject["cloud"]["cycleduration"] = LightController_getValues()->cloud_cycle_duration_min;
    myObject["cloud"]["min"] = LightController_getValues()->min_light_cloudP;
    myObject["cloud"]["max"] = LightController_getValues()->max_light_cloudP;

    return JSON.stringify(myObject);
}

void setup()
{
    // put your setup code here, to run once:
    if (Serial.available())
        Serial.begin(115200);

    vTaskDelay(500);
    log_i("init flash");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS-Partition ist beschädigt oder eine neue Version wurde gefunden
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    log_i("init flash done");
#ifdef USE_SDCARD
    log_i("init filecontroller");
    FileController_setup();
    log_i("init filecontroller done");
#endif
    log_i("connect wifi");
    WiFi.setHostname("Esp32FanController");
    WiFi.mode(WIFI_STA);
    WiFi.begin(MYSSID, PW);

    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(500);
    }
    log_i("connect wifi done");
    log_i("init mdns");
    mdns_init();
    mdns_hostname_set("Esp32FanController");
    mdns_instance_name_set("Esp32FanController");
    mdns_service_add("Esp32FanController", "_http", "_tcp", 80, NULL, 0);
    configTime(time_zone_hour_utc_offset * 60 * 60, 0, "pool.ntp.org");
    log_i("init mdns done");

    MyWebServer_getCallbacksStruct()->applyspeed_listner = FanController_applyspeed;
    MyWebServer_getCallbacksStruct()->voltagechanged_listner = FanController_setVoltage;
    MyWebServer_getCallbacksStruct()->targettemphum_listner = FanController_setTargetTempHumSpeedDif;
    MyWebServer_getCallbacksStruct()->autocontrol_listner = FanController_setAutoControl;
    MyWebServer_getCallbacksStruct()->getFanControllerSettings = getSettings;
#ifdef GOVEE_BTH5179
    MyWebServer_getCallbacksStruct()->readgovee_listner = GoveeBTh5179_enable;
#endif
#ifdef SENSOR_ENS160AHT21
    MyWebServer_getCallbacksStruct()->setTempHumDif = Ens160Aht2x_setTempHumDif;
#endif
#ifdef SENSOR_BME280
#ifndef SENSOR_ENS160AHT21
    MyWebServer_getCallbacksStruct()->setTempHumDif = Bme280_setTempHumDif;
#endif
#endif
    MyWebServer_getCallbacksStruct()->setMinMaxSpeed = FanController_setMinMaxFanSpeed;
    MyWebServer_getCallbacksStruct()->fancoltroller_nightmodeactivcecallback = FanController_setNightMode;
    MyWebServer_getCallbacksStruct()->fancoltroller_nightmodecallback = FanController_setNightModeValues;
    MyWebServer_getCallbacksStruct()->lightController_setLight = LightController_setLight;
    MyWebServer_getCallbacksStruct()->lightController_setTimes = LightController_setTimes;
    MyWebServer_getCallbacksStruct()->lightController_setVoltageLimits = LightController_setVoltageLimits;
    MyWebServer_getCallbacksStruct()->lightController_setAuto = LightController_setAutoMode;
    MyWebServer_getCallbacksStruct()->lightController_setPercentLimits = LightController_setPercentLimits;
    MyWebServer_getCallbacksStruct()->lightController_setCloudActive = LightController_setCloudActive;
    MyWebServer_getCallbacksStruct()->lightController_setCloudValues = LightController_setCloudValues;
#ifdef USE_SDCARD
    MyWebServer_getCallbacksStruct()->fileController_read = FileController_read;
#endif
    MyWebServer_setup();

#ifdef SENSOR_ENS160AHT21
    log_i("init Ens160Aht2x");
    Ens160Aht2x_setup();
#ifndef SENSOR_BME280
    FanController_setHumidityAndTempFunctions(Ens160Aht2x_getHumidity, Ens160Aht2x_getTemperature);
    FanController_setAvgHumidityAndTempFunctions(Ens160Aht2x_getAvarageHumidity, Ens160Aht2x_getAvarageTemperature);
#endif
    log_i("init Ens160Aht2x done");
#endif

#ifdef SENSOR_BME280
    log_i("init bme280");
    Bme280_setup();

    FanController_setHumidityAndTempFunctions(Bme280_getHumidity, Bme280_getTemperature);
    FanController_setAvgHumidityAndTempFunctions(Bme280_getAvarageHumidity, Bme280_getAvarageTemperature);
    log_i("init bme280 done");
#endif
    log_i("setup fancontroller");
    FanController_setup();
    log_i("setup lightcontroller");
    LightController_setup();
#ifdef GOVEE_BTH5179
    GoveeBTh5179_setEventListner(govee_dataListner);
    log_i("setup GoveeBTh5179");
    GoveeBTh5179_setup();
#endif
    log_i("setup done");
}

long startTime;
void loop()
{
    startTime = millis();
#ifdef GOVEE_BTH5179
    GoveeBTh5179_loop();
#endif
    FanController_loop();

    LightController_loop();
#ifdef SENSOR_BME280
    Bme280_loop();
#ifndef SENSOR_ENS160AHT21
#ifdef USE_SDCARD
    FileController_write(Bme280_getAvarageTemperature(), Bme280_getAvarageHumidity(), FanController_getValues()->autocontrolfanspeed, 0, LightController_getValues()->voltage.voltage, Bme280_getVpdLeaf());
#endif
#endif
#endif
#ifdef SENSOR_ENS160AHT21
    Ens160Aht2x_loop();
#ifdef USE_SDCARD
    FileController_write(Ens160Aht2x_getAvarageTemperature(), Ens160Aht2x_getAvarageHumidity(), FanController_getValues()->autocontrolfanspeed, Ens160Aht2x_getCo2(), LightController_getValues()->voltage.voltage, Ens160Aht2x_getVpdAir());
#endif
#endif
    if (MyWebServer_WsClientsConnected())
        sendSocketMsg();
    long end = 1000 - (millis() - startTime);
    if (end < 0)
        end = 1000;
    vTaskDelay(end);
}
