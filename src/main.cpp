#include <Arduino.h>
#include <FS.h>
#include <WiFi.h>
#include "config.h"
#include "mdns.h"
#include "FanController.h"
#include "Arduino_JSON.h"
#include "GoveeBTh5179.h"
#include "Ens160Aht2x.h"
#include "MyWebServer.h"
#include "time.h"
#include "LightController.h"
#include "FileController.h"

void govee_dataListner(double temp, double hum, int bat)
{
    JSONVar socketmsg;
    char buf[64];
    snprintf(buf, sizeof buf, "%.2f", temp);
    socketmsg["temperatur"] = buf;
    snprintf(buf, sizeof buf, "%.2f", hum);
    socketmsg["humidity"] = buf;
    socketmsg["battery"] = bat;
    MyWebServer_sendSocketMsg(JSON.stringify(socketmsg));
}

void ens160Ath2x_dataListner(double temp, double humidity, int aqi, int tvoc, int eco2)
{
    JSONVar socketmsg;
    char buf[64];
    int ret = snprintf(buf, sizeof buf, "%.2f", temp);
    socketmsg["temperatur"] = buf;
    ret = snprintf(buf, sizeof buf, "%.2f", humidity);
    socketmsg["humidity"] = buf;
    ret = snprintf(buf, sizeof buf, "%.2f", (Ens160Aht2x_getAvarageTemperature()));
    socketmsg["atemperatur"] = buf;
    ret = snprintf(buf, sizeof buf, "%.2f", (Ens160Aht2x_getAvarageHumidity()));
    socketmsg["ahumidity"] = buf;
    socketmsg["eco2"] = eco2;
    socketmsg["aqi"] = aqi;
    socketmsg["tvoc"] = tvoc;
    if (FanController_getValues()->autocontrol)
    {
        socketmsg["autocontrolspeed"] = FanController_getValues()->autocontrolfanspeed;
        socketmsg["voltage0"] = FanController_getFan0()->voltage;
        socketmsg["voltage1"] = FanController_getFan1()->voltage;
    }
    socketmsg["nightmode"] = FanController_getValues()->nightmodeActive;
    tm time;
    getLocalTime(&time);
    ret = snprintf(buf, sizeof buf, "%02i:%02i:%02i", time.tm_hour, time.tm_min, time.tm_sec);
    socketmsg["time"] = buf;
    socketmsg["lightvalP"] = LightController_getValues()->currentLightP;
    socketmsg["lightvalmv"] = LightController_getValues()->voltage.voltage;
    socketmsg["lightstate"] = LightController_getValues()->current_state;
    socketmsg["vpdair"] = Ens160Aht2x_getVpdAir();
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
    myObject["readgovee"] = GoveeBTh5179_isEnable();
    myObject["speeddif"] = FanController_getValues()->filtercompensation;
    myObject["tempdif"] = Ens160Aht2x_getTemperatureDif();
    myObject["humdif"] = Ens160Aht2x_getHumidityDif();
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

    return JSON.stringify(myObject);
}

void setup()
{
    // put your setup code here, to run once:
    if (Serial.available())
        Serial.begin(115200);

    FileController_setup();

    WiFi.setHostname("Esp32 FanController");
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PW);

    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(500);
    }
    configTime(2 * 60 * 60, 0, "pool.ntp.org");

    MyWebServer_getCallbacksStruct()->applyspeed_listner = FanController_applyspeed;
    MyWebServer_getCallbacksStruct()->voltagechanged_listner = FanController_setVoltage;
    MyWebServer_getCallbacksStruct()->targettemphum_listner = FanController_setTargetTempHumSpeedDif;
    MyWebServer_getCallbacksStruct()->autocontrol_listner = FanController_setAutoControl;
    MyWebServer_getCallbacksStruct()->getFanControllerSettings = getSettings;
    MyWebServer_getCallbacksStruct()->readgovee_listner = GoveeBTh5179_enable;
    MyWebServer_getCallbacksStruct()->setTempHumDif = Ens160Aht2x_setTempHumDif;
    MyWebServer_getCallbacksStruct()->setMinMaxSpeed = FanController_setMinMaxFanSpeed;
    MyWebServer_getCallbacksStruct()->fancoltroller_nightmodeactivcecallback = FanController_setNightMode;
    MyWebServer_getCallbacksStruct()->fancoltroller_nightmodecallback = FanController_setNightModeValues;
    MyWebServer_getCallbacksStruct()->lightController_setLight = LightController_setLight;
    MyWebServer_getCallbacksStruct()->lightController_setTimes = LightController_setTimes;
    MyWebServer_getCallbacksStruct()->lightController_setVoltageLimits = LightController_setVoltageLimits;
    MyWebServer_getCallbacksStruct()->lightController_setAuto = LightController_setAutoMode;
    MyWebServer_setup();

    Ens160Aht2x_setDataListner(ens160Ath2x_dataListner);
    Ens160Aht2x_setup();

    FanController_setHumidityAndTempFunctions(Ens160Aht2x_getHumidity, Ens160Aht2x_getTemperature);
    FanController_setAvgHumidityAndTempFunctions(Ens160Aht2x_getAvarageHumidity, Ens160Aht2x_getAvarageTemperature);
    FanController_setup();

    LightController_setup();

    mdns_init();
    mdns_hostname_set("Esp32 FanController");
    mdns_instance_name_set("Esp32 FanController");
    mdns_service_add("Esp32 FanController", "_http", "_tcp", 80, NULL, 0);

    GoveeBTh5179_setEventListner(govee_dataListner);
    GoveeBTh5179_setup();
}


void loop()
{
    /*if(voltage == 0)
      voltage = 5000;
    else if(voltage == 5000)
      voltage = 0;
    dac.setDACOutVoltage(voltage, 0);
    //dac.store();
    vTaskDelay(5000);
    //dac.store();
     log_i("loop");*/
    GoveeBTh5179_loop();
    if (FanController_getValues()->autocontrol)
    {
        FanController_processAutoControl();
    }
    FanController_loop();
    Ens160Aht2x_loop();
    LightController_loop();
    FileController_write(Ens160Aht2x_getAvarageTemperature(), Ens160Aht2x_getAvarageHumidity(), FanController_getValues()->autocontrolfanspeed, Ens160Aht2x_getCo2(), LightController_getValues()->voltage.voltage, Ens160Aht2x_getVpdAir());

    vTaskDelay(1000);
}
