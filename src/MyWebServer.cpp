#include "WebServer.h"
#include "ESPAsyncWebServer.h"
#include "MyWebServer.h"
#include "SPIFFS.h"
#include "config.h"
#include "SD.h"

AsyncWebServer *server;
AsyncWebSocket *ws;

MyWebServerMethodCallbacks methcallbacks;

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
                methcallbacks.applyspeed_listner(0, id, val);
            else if (id == 1)
                methcallbacks.applyspeed_listner(0, id, val);
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
        if (methcallbacks.voltagechanged_listner != nullptr)
            methcallbacks.voltagechanged_listner(id, min.toInt(), max.toInt());
        request->send(200);
    }
    else if (variable == "autovals")
    {
        String temp = request->arg("temp");
        String hum = request->arg("hum");
        String spe = request->arg("speeddif");
        if (methcallbacks.targettemphum_listner != nullptr)
            methcallbacks.targettemphum_listner(temp.toInt(), hum.toInt(), spe.toInt());
        request->send(200);
    }
    else if (variable == "autocontrol")
    {
        String autoc = request->arg("val");
        log_i("autocontrol %s", autoc.c_str());
        if (methcallbacks.autocontrol_listner != nullptr)
            methcallbacks.autocontrol_listner(autoc.toInt());
        request->send(200);
    }
    else if (variable == "readgovee")
    {
        String autoc = request->arg("val");
        log_i("readgovee %s", autoc.c_str());
        if (methcallbacks.readgovee_listner != nullptr)
            methcallbacks.readgovee_listner(autoc.toInt());
        request->send(200);
    }
    else if (variable == "temphumdif")
    {
        String tmp = request->arg("temp");
        String hum = request->arg("hum");
        if (methcallbacks.setTempHumDif != nullptr)
            methcallbacks.setTempHumDif(tmp.toFloat(), hum.toFloat());
        request->send(200);
    }
    else if (variable == "autospeed")
    {
        String min = request->arg("min");
        String max = request->arg("max");
        if (methcallbacks.setMinMaxSpeed != nullptr)
        {
            methcallbacks.setMinMaxSpeed(min.toInt(), max.toInt());
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
        if (methcallbacks.fancoltroller_nightmodecallback != nullptr)
            methcallbacks.fancoltroller_nightmodecallback(onh.toInt(), onm.toInt(), offh.toInt(), offm.toInt(), mspeed.toInt());
        request->send(200);
    }
    else if(variable == "fannightmodeactive")
    {
        String on = request->arg("nighton");
        if(methcallbacks.fancoltroller_nightmodeactivcecallback != nullptr)
            methcallbacks.fancoltroller_nightmodeactivcecallback(on.toInt());
        request->send(200);
    }
    else if(variable == "lightvoltage")
    {
        String min = request->arg("min");
        String max = request->arg("max");
        if(methcallbacks.lightController_setVoltageLimits != nullptr)
            methcallbacks.lightController_setVoltageLimits(min.toInt(), max.toInt());
        request->send(200);
    }
    else if(variable == "lightval")
    {
        String val = request->arg("val");
        if(methcallbacks.lightController_setLight != nullptr)
            methcallbacks.lightController_setLight(val.toInt());
        request->send(200);
    }
    else if(variable == "lightsettime")
    {
        String onh = request->arg("onh");
        String onmin = request->arg("onmin");
        String offh = request->arg("offh");
        String offmin = request->arg("offmin");
        String riseh = request->arg("riseh");
        String risemin = request->arg("risemin");
        String seth = request->arg("seth");
        String setmin = request->arg("setmin");
        String riseenable = request->arg("riseenable");
        String setenable = request->arg("setenable");
        if(methcallbacks.lightController_setTimes != nullptr)
            methcallbacks.lightController_setTimes(onh.toInt(),onmin.toInt(),offh.toInt(),offmin.toInt(),riseh.toInt(),risemin.toInt(),seth.toInt(), setmin.toInt(), riseenable.toInt(), setenable.toInt());
        request->send(200);
    }
    else if(variable == "lightautomode")
    {
        String enable = request->arg("enable");
        if(methcallbacks.lightController_setAuto != nullptr)
            methcallbacks.lightController_setAuto(enable.toInt());
        request->send(200);
    }
    else
        request->send(404);
}

void onGetSettings(AsyncWebServerRequest *request)
{
    request->send(200, "text/json", methcallbacks.getFanControllerSettings());
}

MyWebServerMethodCallbacks * MyWebServer_getCallbacksStruct()
{
    return &methcallbacks;
}

void MyWebServer_setup()
{
    SPIFFS.begin();
    server = new AsyncWebServer(http_port);
    server->on("/cmd", HTTP_GET, onCmd);
    server->on("/settings", HTTP_GET, onGetSettings);
    server->serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
    server->serveStatic("/", SD, "/");
    //server->serveStatic("/", SPIFFS, "/www/");
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
