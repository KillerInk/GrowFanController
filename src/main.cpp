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

void govee_dataListner(float temp, float hum, int bat)
{
    JSONVar socketmsg;
    char buf[64];
    snprintf(buf, sizeof buf, "%.2f", temp);
    socketmsg["temperatur"] = buf;
    snprintf(buf, sizeof buf, "%.2f", hum);
    socketmsg["humidity"] = buf;
    socketmsg["battery"] = bat;
    MyWebServer_sendSocketMsg(JSON.stringify(socketmsg));
    socketmsg.~JSONVar();
}

void ens160Ath2x_dataListner(float temp, float humidity, int aqi, int tvoc, int eco2)
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
    MyWebServer_sendSocketMsg(JSON.stringify(socketmsg));
    socketmsg.~JSONVar();
}

String getSettings()
{
    JSONVar myObject;
    myObject["fan0voltage"] = FanController_getFan0()->voltage;
    myObject["fan0min"] = FanController_getFan0()->min;
    myObject["fan0max"] = FanController_getFan0()->max;
    myObject["fan1voltage"] = FanController_getFan1()->voltage;
    myObject["fan1min"] = FanController_getFan1()->min;
    myObject["fan1max"] = FanController_getFan0()->max;
    myObject["autocontrol"] = FanController_getValues()->autocontrol;
    myObject["targetTemperature"] = FanController_getValues()->targetTemperature;
    myObject["targetHumidity"] = FanController_getValues()->targetHumidity;
    myObject["readgovee"] = GoveeBTh5179_isEnable();
    myObject["speeddif"] = FanController_getValues()->filtercompensation;
    myObject["tempdif"] = Ens160Aht2x_getTemperatureDif();
    myObject["humdif"] = Ens160Aht2x_getHumidityDif();
    myObject["minspeed"] = FanController_getValues()->minspeed;
    myObject["maxspeed"] = FanController_getValues()->maxspeed;

    return JSON.stringify(myObject);
    myObject.~JSONVar();
}

void setup()
{
    // put your setup code here, to run once:
    if (Serial.available())
        Serial.begin(115200);

    WiFi.setHostname("Esp32 FanController");
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PW);

    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(500);
    }
    configTime(2*60*60, 0, "pool.ntp.org");

    MyWebServer_setApplySpeedListner(FanController_applyspeed);
    MyWebServer_setVoltageChangedListner(FanController_setVoltage);
    MyWebServer_setTargetTempHumChangedListner(FanController_setTargetTempHumSpeedDif);
    MyWebServer_setAutoControlListner(FanController_setAutoControl);
    MyWebServer_setFanControllerGetSettings(getSettings);
    MyWebServer_setReadGoveeListner(GoveeBTh5179_enable);
    MyWebServer_setTempHumDif(Ens160Aht2x_setTempHumDif);
    MyWebServer_setMinMaxSpeed(FanController_setMinMaxFanSpeed);
    MyWebServer_setup();

    Ens160Aht2x_setDataListner(ens160Ath2x_dataListner);
    Ens160Aht2x_setup();

    FanController_setHumidityAndTempFunctions(Ens160Aht2x_getHumidity, Ens160Aht2x_getTemperature);
    FanController_setAvgHumidityAndTempFunctions(Ens160Aht2x_getAvarageHumidity, Ens160Aht2x_getAvarageTemperature);
    FanController_setup();

    //LightController_setup();

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
    Ens160Aht2x_loop();
    //LightController_loop();

    vTaskDelay(1000);
}
