#include "WebServer.h"
#include "ESPAsyncWebServer.h"
#include "MyWebServer.h"
#include "SPIFFS.h"
#include "config.h"

AsyncWebServer *server;
AsyncWebSocket *ws;
void (*applyspeed_listner)(int volt, int id, int val);
void (*voltagechanged_listner)(int id, int min, int max);
void (*targettemphum_listner)(int tmp, int hum, int speed);
void (*autocontrol_listner)(bool enable);
String (*getFanControllerSettings)();
void (*readgovee_listner)(bool enable);
void (*setTempHumDif)(float temp, float hum);
void (*setMinMaxSpeed)(int min, int max);
void (*fancoltroller_nightmodecallback)(int onhour, int onmin, int offhour, int offmin, int maxspeed);
void (*fancoltroller_nightmodeactivcecallback)(bool active);

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
                applyspeed_listner(0, id, val);
            else if (id == 1)
                applyspeed_listner(0, id, val);
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
        if (voltagechanged_listner != nullptr)
            voltagechanged_listner(id, min.toInt(), max.toInt());
        request->send(200);
    }
    else if (variable == "autovals")
    {
        String temp = request->arg("temp");
        String hum = request->arg("hum");
        String spe = request->arg("speeddif");
        if (targettemphum_listner != nullptr)
            targettemphum_listner(temp.toInt(), hum.toInt(), spe.toInt());
        request->send(200);
    }
    else if (variable == "autocontrol")
    {
        String autoc = request->arg("val");
        log_i("autocontrol %s", autoc.c_str());
        if (autocontrol_listner != nullptr)
            autocontrol_listner(autoc.toInt());
        request->send(200);
    }
    else if (variable == "readgovee")
    {
        String autoc = request->arg("val");
        log_i("readgovee %s", autoc.c_str());
        if (readgovee_listner != nullptr)
            readgovee_listner(autoc.toInt());
        request->send(200);
    }
    else if (variable == "temphumdif")
    {
        String tmp = request->arg("temp");
        String hum = request->arg("hum");
        if (setTempHumDif != nullptr)
            setTempHumDif(tmp.toFloat(), hum.toFloat());
        request->send(200);
    }
    else if (variable == "autospeed")
    {
        String min = request->arg("min");
        String max = request->arg("max");
        if (setMinMaxSpeed != nullptr)
        {
            setMinMaxSpeed(min.toInt(), max.toInt());
        }
        request->send(200);
    }
    else if (variable == "fannightmode")
    {
        String onh = request->arg("onh");
        String onm = request->arg("onm");
        String offh = request->arg("offh");
        String offm = request->arg("offm");
        String mspeed = request->arg("mspeed");
        if (fancoltroller_nightmodecallback != nullptr)
            fancoltroller_nightmodecallback(onh.toInt(), onm.toInt(), offh.toInt(), offm.toInt(), mspeed.toInt());
        request->send(200);
    }
    else if(variable == "fannightmodeactive")
    {
        String on = request->arg("nighton");
        if(fancoltroller_nightmodeactivcecallback != nullptr)
            fancoltroller_nightmodeactivcecallback(on.toInt());
    }
    else
        request->send(404);
}

void onGetSettings(AsyncWebServerRequest *request)
{
    request->send(200, "text/json", getFanControllerSettings());
}

void MyWebServer_setup()
{
    SPIFFS.begin();
    server = new AsyncWebServer(http_port);
    server->on("/cmd", HTTP_GET, onCmd);
    server->on("/settings", HTTP_GET, onGetSettings);
    server->serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
    server->serveStatic("/script.js", SPIFFS, "/www/script.js");
    server->serveStatic("/style.css", SPIFFS, "/www/style.css");
    ws = new AsyncWebSocket("/ws");
    ws->onEvent(onWsEvent);
    server->addHandler(ws);
    server->begin();
    log_i("Started Webserver on port %i", http_port);
}
void MyWebServer_sendSocketMsg(String msg)
{
    ws->textAll(msg);
}

void MyWebServer_setApplySpeedListner(void func(int volt, int id, int val))
{
    applyspeed_listner = func;
}

void MyWebServer_setVoltageChangedListner(void func(int id, int min, int max))
{
    voltagechanged_listner = func;
}

void MyWebServer_setTargetTempHumChangedListner(void func(int tmp, int hum, int speed))
{
    targettemphum_listner = func;
}

void MyWebServer_setAutoControlListner(void func(bool enable))
{
    autocontrol_listner = func;
}

void MyWebServer_setFanControllerGetSettings(String func())
{
    getFanControllerSettings = func;
}

void MyWebServer_setReadGoveeListner(void func(bool enable))
{
    readgovee_listner = func;
}

void MyWebServer_setTempHumDif(void func(float temp, float hum))
{
    setTempHumDif = func;
}

void MyWebServer_setMinMaxSpeed(void func(int min, int max))
{
    setMinMaxSpeed = func;
}

void MyWebServer_setFanControllerNightModeCallback(void func(int onhour, int onmin, int offhour, int offmin, int maxspeed))
{
    fancoltroller_nightmodecallback = func;
}

void MyWebServer_setFanControllerNightModeActiveCallback(void func(bool active))
{
    fancoltroller_nightmodeactivcecallback = func;
}
