#include <Arduino.h>
#include "DFRobot_GP8403.h"
#include <FS.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "config.h"
#include "SPIFFS.h"
#include "mdns.h"

DFRobot_GP8403 dac(&Wire, 0x5F);
uint16_t voltage = 0;
uint16_t voltage1 = 0;
AsyncWebServer *server;
int minVoltage = 800;  // hz
int maxVoltage = 1100; // hz

void applyspeed(int volt, int id,int val)
{
  if (val > 0)
    {
      u_int16_t s = minVoltage + (float)(maxVoltage - minVoltage) * ((float)val / 100);
      volt = s;
    }
    else
      volt = 0;

    log_i("set id:%i voltage to %i", id, volt);
    dac.setDACOutVoltage(volt, id);
}

void onCmd(AsyncWebServerRequest *request)
{

  String variable = request->arg("var");
  String value = request->arg("val");
  String ids = request->arg("id");
  int val = value.toInt();
  int id = ids.toInt();

  log_i("set %s to %s", variable.c_str(), value.c_str());

  if (variable == "speed" && val <= 100 && val >= 0)
  {
    if(id == 0)
      applyspeed(voltage,id,val);
    else if (id == 1)
      applyspeed(voltage1,id,val);
    request->send(200);
  }
  else
    request->send(404);
}

void setup()
{
  // put your setup code here, to run once:
  if (Serial.available())
    Serial.begin(115200);
  dac.begin(I2C_SDA, I2C_SCL);

  WiFi.setHostname("Esp32 FanController");
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PW);

  while (WiFi.status() != WL_CONNECTED)
  {
    vTaskDelay(500);
  }

  SPIFFS.begin();
  server = new AsyncWebServer(http_port);
  server->on("/cmd", HTTP_GET, onCmd);
  server->serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
  server->begin();
  log_i("Started Webserver on port %i", http_port);

  // Set DAC output range
  dac.setDACOutRange(dac.eOutputRange10V);
  // Set the output value for DAC channel 0, range 0-10000

  // Store data in the chip
  // dac.store();
  mdns_init();
  mdns_hostname_set("Esp32 FanController");
  mdns_instance_name_set("Esp32 FanController");
  mdns_service_add("Esp32 FanController", "_http", "_tcp", 80, NULL, 0);
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
}
