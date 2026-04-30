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
#include "WiFiManager.h"
#include "MyPreferences.h"

// Runtime timezone offset (hours from UTC, default compile-time value)
static int g_timeZoneOffset = time_zone_hour_utc_offset;

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

// Helper: convert lifecycle_stage enum to int for JSON
static int stageEnumToInt(lifecycle_stage s) {
    return (int)s;
}

// Helper: get stage name string
static const char* getStageName(lifecycle_stage s) {
    switch (s) {
        case stage_seedling: return "seedling";
        case stage_vegetative: return "vegetative";
        case stage_flower_early: return "flower_early";
        case stage_flower_late: return "flower_late";
        case stage_maturation: return "maturation";
        default: return "unknown";
    }
}

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
    ret = snprintf(buf, sizeof buf, "%.2f", Bme280_getVpdLeaf());
    socketmsg["bme280"]["vpd"] = buf;

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
    ret = snprintf(buf, sizeof buf, "%.2f", Ens160Aht2x_getVpdLeaf());
    socketmsg["ens160aht21"]["vpd"] = buf;
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

    // Lifecycle/PPFD/DLI socket data (added 2026-04-10)
    auto *lc = &LightController_getValues()->lifecycle;
    // Compute currentLightTargetP if lifecycle is enabled
    if (lc->enabled) {
        calculateLifecycleLightP(lc, 0);
    }
    socketmsg["lifecycle"]["enabled"] = lc->enabled;
    socketmsg["lifecycle"]["stage"] = stageEnumToInt(lc->stage);
    socketmsg["lifecycle"]["stageName"] = getStageName(lc->stage);
    socketmsg["lifecycle"]["stageDay"] = lc->stageDay;
    socketmsg["lifecycle"]["currentLightTargetP"] = lc->currentLightTargetP;
    socketmsg["lifecycle"]["panelMaxPPFD"] = lc->panelMaxPPFD;
    socketmsg["lifecycle"]["accumulatedDLI"] = lc->accumulatedDLI;

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

    // Lifecycle settings (added 2026-04-10)
    auto *lc = &LightController_getValues()->lifecycle;
    // Compute currentLightTargetP if lifecycle is enabled
    if (lc->enabled) {
        calculateLifecycleLightP(lc, 0);
    }
    myObject["lifecycle"]["enabled"] = lc->enabled;
    myObject["lifecycle"]["stage"] = stageEnumToInt(lc->stage);
    myObject["lifecycle"]["stageName"] = getStageName(lc->stage);
    myObject["lifecycle"]["stageDay"] = lc->stageDay;
    myObject["lifecycle"]["stageStartTimestamp"] = lc->stageStartTimestamp;
    myObject["lifecycle"]["accumulatedDLI"] = lc->accumulatedDLI;
    myObject["lifecycle"]["panelMaxPPFD"] = lc->panelMaxPPFD;
    myObject["lifecycle"]["umolPerWatt"] = lc->umolPerWatt;
    myObject["lifecycle"]["currentLightTargetP"] = lc->currentLightTargetP;

    // System info
    unsigned long uptimeMs = millis();
    unsigned long uptimeDays = uptimeMs / 86400000;
    unsigned long uptimeHours = (uptimeMs % 86400000) / 3600000;
    unsigned long uptimeMinutes = (uptimeMs % 3600000) / 60000;
    unsigned long uptimeSeconds = (uptimeMs % 60000) / 1000;
    myObject["uptime"] = String(uptimeDays) + "d " + String(uptimeHours) + "h " + String(uptimeMinutes) + "m " + String(uptimeSeconds) + "s";
    myObject["freeHeap"] = ESP.getFreeHeap();
    myObject["minFreeHeap"] = ESP.getMinFreeHeap();
    myObject["maxAlloc"] = ESP.getMaxAllocHeap();
    myObject["chipid"] = String(ESP.getChipModel());
    myObject["firmwareVersion"] = String(FIRMWARE_VERSION);
    myObject["chipmodel"] = ESP.getChipModel();
    myObject["spisize"] = ESP.getFlashChipSize();
    myObject["spiflashspeed"] = ESP.getFlashChipSpeed();
    myObject["spiflsize"] = ESP.getFlashChipSize();

    // WiFi status
    WifiStatus wifiStatus = wifiManager.getStatus();
    myObject["wifi_connected"] = wifiStatus.connected;
    myObject["wifi_rssi"] = wifiStatus.rssi;
    myObject["wifi_ssid"] = wifiStatus.ssid[0] ? String(wifiStatus.ssid) : "";
    myObject["ap_active"] = wifiStatus.apActive;
    myObject["ap_ssid"] = wifiStatus.apActive ? String(wifiStatus.apSsid) : "";
    myObject["ap_ip"] = wifiStatus.apActive ? String(wifiStatus.apIp) : "";

    // Timezone offset
    myObject["timezoneOffset"] = g_timeZoneOffset;

    return JSON.stringify(myObject);
}

String wificonfig()
{
    return wifiManager.getWifiConfigPage();
}

void wifipost(AsyncWebServerRequest *request)
{
    wifiManager.handleWifiConfigPost(request);
}

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);

    vTaskDelay(500);
    log_i("init flash");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS-Partition ist beschadigt oder eine neue Version wurde gefunden
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    log_i("init flash done");
    // Load timezone offset from NVS (falls back to compile-time default)
    g_timeZoneOffset = MyPreferences_getTimeZoneOffset();
#ifdef USE_SDCARD
    log_i("init filecontroller");
    FileController_setup();
    log_i("init filecontroller done");
#endif
    log_i("connect wifi");
    WiFi.setHostname("Esp32FanController");
    
    // Initialize WiFiManager (handles NVS, connection, AP fallback)
    wifiManager.setup();
    log_i("WiFiManager initialized");
    log_i("init mdns");
    mdns_init();
    mdns_hostname_set("Esp32FanController");
    mdns_instance_name_set("Esp32FanController");
    mdns_service_add("Esp32FanController", "_http", "_tcp", 80, NULL, 0);
    configTime(g_timeZoneOffset * 60 * 60, 0, "pool.ntp.org");
    log_i("Timezone offset: %d hours", g_timeZoneOffset);
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
    // Lifecycle callbacks (added 2026-04-10)
    MyWebServer_getCallbacksStruct()->lightController_setLifecycleEnabled = LightController_setLifecycleEnabled;
    MyWebServer_getCallbacksStruct()->lightController_setLifecycleStage = [](int stage) { LightController_setLifecycleStage(static_cast<lifecycle_stage>(stage)); };
    MyWebServer_getCallbacksStruct()->lightController_resetLifecycle = LightController_resetLifecycleStage;
    MyWebServer_getCallbacksStruct()->lightController_getLifecycleState = []() -> String {
        auto *lc = &LightController_getValues()->lifecycle;
        // Compute currentLightTargetP if lifecycle is enabled
        if (lc->enabled) {
            calculateLifecycleLightP(lc, 0);
        }
        JSONVar obj;
        obj["enabled"] = lc->enabled;
        obj["stage"] = stageEnumToInt(lc->stage);
        obj["stageName"] = getStageName(lc->stage);
        obj["stageDay"] = lc->stageDay;
        obj["stageStartTimestamp"] = lc->stageStartTimestamp;
        obj["accumulatedDLI"] = lc->accumulatedDLI;
        obj["panelMaxPPFD"] = lc->panelMaxPPFD;
        obj["umolPerWatt"] = lc->umolPerWatt;
        obj["currentLightTargetP"] = lc->currentLightTargetP;
        return JSON.stringify(obj);
    };
    MyWebServer_getCallbacksStruct()->lightController_setPanelPPFD = [](float ppfd) {
        LightController_getValues()->lifecycle.panelMaxPPFD = ppfd;
        MyPreferences_setBytes("light", LightController_getValues(), sizeof(LightControllerValues));
    };
    MyWebServer_getCallbacksStruct()->wifiConfigGet = wificonfig;

    MyWebServer_getCallbacksStruct()->wifiConfigPost = wifipost;
    MyWebServer_getCallbacksStruct()->getTimeZoneOffset = []() {
        return g_timeZoneOffset;
    };
    MyWebServer_getCallbacksStruct()->setTimeZoneOffset = [](int offset) {
        g_timeZoneOffset = offset;
        configTime(g_timeZoneOffset * 60 * 60, 0, "pool.ntp.org");
        MyPreferences_setTimeZoneOffset((int8_t)offset);
    };
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
#ifdef SENSOR_BME280
    Bme280_loop();
#endif
#ifdef SENSOR_ENS160AHT21
    Ens160Aht2x_loop();
#endif
    FanController_loop();

    LightController_loop();
    
    // WiFi manager loop (handles connection timeout, AP fallback, periodic retry)
    wifiManager.loop();
#ifdef USE_SDCARD
    FileController_write();
#endif
    if (MyWebServer_WsClientsConnected())
        sendSocketMsg();
    long end = 1000 - (millis() - startTime);
    if (end < 0)
        end = 1000;
    vTaskDelay(end);
}
