#include <Arduino.h>
#include "DFRobot_GP8403.h"
#include <FS.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "config.h"
#include "SPIFFS.h"
#include "mdns.h"
#include "Preferences.h"
#include "Arduino_JSON.h"
#include "NimBLEDevice.h"
#include "DFRobot_ENS160.h"

#include <AHT20.h>

//[    47][I][main.cpp:310] setup(): found address 56
//[    51][I][main.cpp:310] setup(): found address 83
//[    53][I][main.cpp:310] setup(): found address 95
DFRobot_GP8403 dac(&Wire, i2c_pwn_addr);
uint16_t voltage = 0;
uint16_t voltage1 = 0;
AsyncWebServer *server;
AsyncWebSocket *ws;
Preferences preferences;
int minVoltage = 800;    // hz
int maxVoltage = 1100;   // hz
int minVoltage1 = 0;     // hz
int maxVoltage1 = 10000; // hz
const char *prefNamespace = "Voltage";
float temp = 0.;
float humidity = 0.;
int battery = 0;
long nextScanTime;
const long scanInterval = 60 * 1000;
int targetTemperature = 26;
int targetHumidity = 60;
bool autocontrol = false;
// tracked fanspeed in autocontrol in %
int autocontrolfanspeed = 50; // %
// in blowing fans need to run slower to compensate the resistance from the filter and keep a bit vacuum inside the tent
int filtercompensation = 5; //%

NimBLEScan *pBLEScan;
NimBLEUUID serviceUuid("180a"); // Govee 5179 service UUID

DFRobot_ENS160_I2C ens160(&Wire, /*I2CAddr*/ 0x53);
AHT20 aht20;

struct goveebtdata
{
  char ident[6];
  uint16_t temp;
  uint16_t humidity;
  uint8_t bat;
} btdata;

u_int16_t getVoltageFromPercent(int maxvoltage, int minvoltage, int val)
{
  return minvoltage + (float)(maxvoltage - minvoltage) * ((float)val / 100);
}

void processAutoControl()
{
  if (temp > targetTemperature || humidity > targetHumidity)
  {
    if (autocontrolfanspeed + 2 <= 100)
      autocontrolfanspeed += 2;
    else
      log_i("max speed reached");
  }
  else if (temp < targetTemperature || humidity < targetHumidity)
  {
    if (autocontrolfanspeed - 2 >= 10)
      autocontrolfanspeed -= 2;
    else
      log_i("min speed reached");
  }
  voltage = getVoltageFromPercent(maxVoltage, minVoltage, autocontrolfanspeed);
  voltage1 = getVoltageFromPercent(maxVoltage1, minVoltage1, autocontrolfanspeed - filtercompensation);
  dac.setDACOutVoltage(voltage, 0);
  dac.setDACOutVoltage(voltage1, 1);
  log_i("autocontrol set speed to: %i", autocontrolfanspeed);
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{

  void onDiscovered(BLEAdvertisedDevice *advertisedDevice)
  {
    // log_i("Advertised Device: %s \n", advertisedDevice->toString().c_str());
  }

  /*
  [ 68866][I][main.cpp:43] onResult(): Advertised Device: Name: Govee_H5179_9C4D,, manufacturer data: 0188ec000101760c9a1552, serviceUUID: 0x180a

  [ 68882][I][main.cpp:63] onResult(): Govee_H5179_9C4D
  [ 68887][I][main.cpp:64] onResult():  2.57 31.90 154
  */
  // Called when BLE scan sees BLE advertisement.
  //                    head     hum   tmp  ka   bat
  // manufacturer data: 0188ec00 0101  6c0c ae15 52, serviceUUID: 0x180a
  //                    0188EC00 0101  220B 2C10 53
  //                    0188EC00 0101  180B 2C10 52
  //                    181201121114020
  void onResult(BLEAdvertisedDevice *advertisedDevice)
  {

    // check for Govee 5074 service UUID
    if (advertisedDevice->getServiceUUID() == serviceUuid)
    {
      // log_i("Advertised Device: %s \n", advertisedDevice->toString().c_str());
      //  Desired Govee advert will have 11 byte mfg. data length & leading bytes0x01 0x88 0xec
      if ((advertisedDevice->getManufacturerData().length() == 11) &&
          ((byte)advertisedDevice->getManufacturerData().data()[1] == 0x88) &&
          ((byte)advertisedDevice->getManufacturerData().data()[2] == 0xec))
      {
        char buf[advertisedDevice->getManufacturerData().length()];
        for (int i = 0; i < advertisedDevice->getManufacturerData().length(); i++)
        {
          buf[i] = advertisedDevice->getManufacturerData()[i];
        }
        goveebtdata *data = (goveebtdata *)buf;
        temp = ((float)data->temp) / 100.;
        humidity = ((float)data->humidity) / 100.;
        battery = data->bat;
        log_i("%s", advertisedDevice->getName().c_str());
        log_i("%i %i %i", data->temp, data->humidity, data->bat);
        JSONVar socketmsg;
        socketmsg["temperatur"] = temp;
        socketmsg["humidity"] = humidity;
        socketmsg["battery"] = battery;
        if (autocontrol)
          socketmsg["autocontrolspeed"] = autocontrolfanspeed;
        ws->textAll(JSON.stringify(socketmsg));
      }
    }
    else
    {
      pBLEScan->erase(advertisedDevice->getAddress());
    }
  }
};
MyAdvertisedDeviceCallbacks *btcallback;

void applyspeed(int volt, int id, int val)
{
  if (val > 0)
  {
    if (id == 0)
    {
      u_int16_t s = getVoltageFromPercent(maxVoltage, minVoltage, val);
      volt = s;
    }
    else if (id == 1)
    {
      u_int16_t s = getVoltageFromPercent(maxVoltage1, minVoltage1, val);
      volt = s;
    }
  }
  else
    volt = 0;

  log_i("set id:%i voltage to %i", id, volt);
  dac.setDACOutVoltage(volt, id);
}

void onGetSettings(AsyncWebServerRequest *request)
{
  JSONVar myObject;
  myObject["fan0voltage"] = voltage;
  myObject["fan0min"] = minVoltage;
  myObject["fan0max"] = maxVoltage;
  myObject["fan1voltage"] = voltage1;
  myObject["fan1min"] = minVoltage1;
  myObject["fan1max"] = maxVoltage1;
  myObject["autocontrol"] = autocontrol;
  myObject["targetTemperature"] = targetTemperature;
  myObject["targetHumidity"] = targetHumidity;
  request->send(200, "text/json", JSON.stringify(myObject));
}

void onCmd(AsyncWebServerRequest *request)
{

  String variable = request->arg("var");

  if (variable == "speed")
  {
    String value = request->arg("val");
    String ids = request->arg("id");
    int val = value.toInt();
    int id = ids.toInt();
    if (val <= 100 && val >= 0)
    {
      log_i("set %s to %s", variable.c_str(), value.c_str());
      if (id == 0)
        applyspeed(voltage, id, val);
      else if (id == 1)
        applyspeed(voltage1, id, val);
      request->send(200);
    }
    else
      request->send(400);
  }
  else if (variable == "voltage")
  {
    String ids = request->arg("id");
    String min = request->arg("min");
    String max = request->arg("max");
    int id = ids.toInt();
    preferences.begin(prefNamespace, false);
    if (id == 0)
    {
      minVoltage = min.toInt();
      maxVoltage = max.toInt();
      preferences.putInt("minv0", minVoltage);
      preferences.putInt("maxv0", maxVoltage);
    }
    else if (id == 1)
    {
      minVoltage1 = min.toInt();
      maxVoltage1 = max.toInt();
      preferences.putInt("minv1", minVoltage1);
      preferences.putInt("maxv1", maxVoltage1);
    }
    preferences.end();
    request->send(200);
  }
  else if (variable == "autovals")
  {
    String temp = request->arg("temp");
    String hum = request->arg("hum");
    targetTemperature = temp.toInt();
    targetHumidity = hum.toInt();
    preferences.begin(prefNamespace, false);
    preferences.putInt("tTemp", targetTemperature);
    preferences.putInt("tHum", targetHumidity);
    preferences.end();
    request->send(200);
  }
  else if (variable == "autocontrol")
  {
    String autoc = request->arg("val");
    log_i("autocontrol %s", autoc.c_str());
    autocontrol = autoc.toInt();
    preferences.begin(prefNamespace, false);
    preferences.putInt("autocontrol", autocontrol);
    preferences.end();
    request->send(200);
  }
  else
    request->send(404);
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{

  if (type == WS_EVT_CONNECT)
  {
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
  }
}

uint8_t checkI2C(uint8_t addr)
{
  Wire.beginTransmission(addr);
  Wire.write(OUTPUT_RANGE);
  byte error;
  error = Wire.endTransmission();
  if (error == 0)
    return 1;
  return 0;
}

void setup()
{
  // put your setup code here, to run once:
  if (Serial.available())
    Serial.begin(115200);

  preferences.begin(prefNamespace, false);
  maxVoltage = preferences.getInt("maxv0", maxVoltage);
  minVoltage = preferences.getInt("minv0", minVoltage);
  maxVoltage1 = preferences.getInt("maxv1", maxVoltage1);
  minVoltage1 = preferences.getInt("minv1", minVoltage1);
  targetHumidity = preferences.getInt("tHum", targetHumidity);
  targetTemperature = preferences.getInt("tTemp", targetTemperature);
  autocontrol = preferences.getInt("autocontrol", autocontrol);
  preferences.end();

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
  server->on("/settings", HTTP_GET, onGetSettings);
  server->serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
  ws = new AsyncWebSocket("/ws");
  ws->onEvent(onWsEvent);
  server->addHandler(ws);
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

  NimBLEDevice::setScanDuplicateCacheSize(10);
  NimBLEDevice::init("");
  pBLEScan = NimBLEDevice::getScan(); // create new scan
  // Set the callback for when devices are discovered, include duplicates.
  btcallback = new MyAdvertisedDeviceCallbacks();
  log_i("btcallback  %i", btcallback);
  pBLEScan->setScanCallbacks(btcallback, true);
  pBLEScan->setActiveScan(true); // Set active scanning, this will get more data from the advertiser.
  pBLEScan->setInterval(1000);   // How often the scan occurs / switches channels; in milliseconds,
  pBLEScan->setWindow(900);      // How long to scan during the interval; in milliseconds.
  pBLEScan->setMaxResults(0);    // do not store the scan results, use callback only.

  Wire.begin();
  Wire.setClock(100000);

  for (byte i = 1; i < 127; i++)
  {
    int avail = checkI2C(i);
    if (avail == 1)
      log_i("found address %i", i);
  }
  // Wire.end();
  boolean rdy = 0;
  rdy = aht20.begin();
  log_i("aht avail:%i", rdy);
  rdy = dac.begin();
  log_i("dac avail:%i", rdy);
  // log_i("firmware: %s",ens160.getFirmwareVersion());
  ens160.setPWRMode(ENS160_STANDARD_MODE);
}

int AQI;
int TVOC; // ppb
int eCO2; // ppm
int hp0;  // Ohm
int hp1;  // Ohm
int hp2;  // Ohm
int hp3;  // Ohm

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

  if (autocontrol)
  {
    if (nextScanTime <= millis() && !pBLEScan->isScanning())
    {
      nextScanTime += scanInterval;
      log_i("Restarting BLE scan");
      pBLEScan->start(1000, false);
    }
    processAutoControl();
  }
  if (true)
  {
    temp = aht20.getTemperature();
    humidity = aht20.getHumidity();
    log_i("temp:%f humidity:%f", temp, humidity);
  }
  if (true)
  {
    ens160.setTempAndHum(temp, humidity);
    /*
     *         1-Warm-Up phase, first 3 minutes after power-on.
     *         2-Initial Start-Up phase, first full hour of operation after initial power-on. Only once in the sensor’s lifetime.
     */
    int status = ens160.getENS160Status();
    // Return value range: 1-5 (Corresponding to five levels of Excellent, Good, Moderate, Poor and Unhealthy respectively)
    AQI = (uint8_t)ens160.getAQI();
    TVOC = ens160.getTVOC();
    eCO2 = ens160.getECO2();
    log_i("status:%i tvoc:%i eco2:%i aqi:%i", status, TVOC, eCO2, AQI);
  }
  JSONVar socketmsg;
  socketmsg["temperatur"] = temp;
  socketmsg["humidity"] = humidity;
  socketmsg["eco2"] = eCO2;
  socketmsg["aqi"] = AQI;
  socketmsg["tvoc"] = TVOC;
  ws->textAll(JSON.stringify(socketmsg));

  vTaskDelay(8000);
}
