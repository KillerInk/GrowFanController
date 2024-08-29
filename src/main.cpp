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

void govee_dataListner(float temp, float hum, int bat)
{
  JSONVar socketmsg;
  socketmsg["temperatur"] = temp;
  socketmsg["humidity"] = hum;
  socketmsg["battery"] = bat;
  if (FanController_isAutoControl())
    socketmsg["autocontrolspeed"] = FanController_getAutoSpeed();
  MyWebServer_sendSocketMsg(JSON.stringify(socketmsg));
}

void ens160Ath2x_dataListner(float temp, float humidity, int aqi, int tvoc, int eco2)
{
  JSONVar socketmsg;
  socketmsg["temperatur"] = temp;
  socketmsg["humidity"] = humidity;
  socketmsg["eco2"] = eco2;
  socketmsg["aqi"] = aqi;
  socketmsg["tvoc"] = tvoc;
  MyWebServer_sendSocketMsg(JSON.stringify(socketmsg));
}

String getSettings()
{
    JSONVar myObject;
    myObject["fan0voltage"] = getVoltage();
    myObject["fan0min"] = getMinVoltage();
    myObject["fan0max"] = getMaxVoltage();
    myObject["fan1voltage"] = getVoltage1();
    myObject["fan1min"] = getMinVoltage1();
    myObject["fan1max"] = getMaxVoltage1();
    myObject["autocontrol"] = FanController_isAutoControl();
    myObject["targetTemperature"] = getTargetTemperature();
    myObject["targetHumidity"] = getTargetHumidity();
    myObject["readgovee"] = GoveeBTh5179_isEnable();
    myObject["speeddif"] = getSpeedDifference();
    myObject["tempdif"] = Ens160Aht2x_getTemperatureDif();
    myObject["humdif"] = Ens160Aht2x_getHumidityDif();
    return JSON.stringify(myObject);
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

  MyWebServer_setApplySpeedListner(FanController_applyspeed);
  MyWebServer_setVoltageChangedListner(FanController_setVoltage);
  MyWebServer_setTargetTempHumChangedListner(FanController_setTargetTempHumSpeedDif);
  MyWebServer_setAutoControlListner(FanController_setAutoControl);
  MyWebServer_setFanControllerGetSettings(getSettings);
  MyWebServer_setReadGoveeListner(GoveeBTh5179_enable);
  MyWebServer_setTempHumDif(Ens160Aht2x_setTempHumDif);
  MyWebServer_setup();

  Ens160Aht2x_setDataListner(ens160Ath2x_dataListner);
  Ens160Aht2x_setup();

  FanController_setHumidityAndTempFunctions(Ens160Aht2x_getHumidity,Ens160Aht2x_getTemperature);
  FanController_setup();

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
  if (FanController_isAutoControl())
  {
    FanController_processAutoControl();
  }
  Ens160Aht2x_loop();
  
  vTaskDelay(8000);
}
