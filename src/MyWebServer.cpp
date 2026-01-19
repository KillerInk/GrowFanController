#include "WebServer.h"
#include "ESPAsyncWebServer.h"
#include "MyWebServer.h"
#include "SPIFFS.h"
#include <esp_partition.h>
#include "SD.h"
#include <FS.h>
#include <esp_ota_ops.h>

AsyncWebServer *server;
AsyncWebSocket *ws;

MyWebServerMethodCallbacks methcallbacks;

int ws_clients = 0;

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{

    if (type == WS_EVT_CONNECT)
    {
        Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
        ws_clients++;
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
        ws_clients--;
    }
    else if (type == WS_EVT_ERROR)
    {
        Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
        if (ws_clients > 0)
            ws_clients--;
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
    log_i("Command: %s", variable);
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
                methcallbacks.applyspeed_listner(id, val);
            else if (id == 1)
                methcallbacks.applyspeed_listner(id, val);
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
            methcallbacks.setTempHumDif(tmp.toDouble(), hum.toDouble());
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
    else if (variable == "fannightmodeactive")
    {
        String on = request->arg("nighton");
        if (methcallbacks.fancoltroller_nightmodeactivcecallback != nullptr)
            methcallbacks.fancoltroller_nightmodeactivcecallback(on.toInt());
        request->send(200);
    }
    else if (variable == "lightvoltage")
    {
        String min = request->arg("min");
        String max = request->arg("max");
        if (methcallbacks.lightController_setVoltageLimits != nullptr)
            methcallbacks.lightController_setVoltageLimits(min.toInt(), max.toInt());
        request->send(200);
    }
    else if (variable == "lightval")
    {
        String val = request->arg("val");
        if (methcallbacks.lightController_setLight != nullptr)
            methcallbacks.lightController_setLight(val.toInt());
        request->send(200);
    }
    else if (variable == "lightlimitsp")
    {
        String val = request->arg("min");
        String max = request->arg("max");
        if (methcallbacks.lightController_setPercentLimits != nullptr)
            methcallbacks.lightController_setPercentLimits(val.toInt(), max.toInt());
        request->send(200);
    }
    else if (variable == "lightsettime")
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
        if (methcallbacks.lightController_setTimes != nullptr)
            methcallbacks.lightController_setTimes(onh.toInt(), onmin.toInt(), offh.toInt(), offmin.toInt(), riseh.toInt(), risemin.toInt(), seth.toInt(), setmin.toInt(), riseenable.toInt(), setenable.toInt());
        request->send(200);
    }
    else if (variable == "lightautomode")
    {
        String enable = request->arg("enable");
        if (methcallbacks.lightController_setAuto != nullptr)
            methcallbacks.lightController_setAuto(enable.toInt());
        request->send(200);
    }
    else if (variable == "cloudsim")
    {
        String min = request->arg("min");
        String max = request->arg("max");
        String duration = request->arg("cloudduration");
        if (methcallbacks.lightController_setCloudValues != nullptr)
            methcallbacks.lightController_setCloudValues(min.toInt(), max.toInt(), duration.toInt());
        request->send(200);
    }
    else if (variable == "cloudsimactive")
    {
        String on = request->arg("val");
        if (methcallbacks.lightController_setCloudActive != nullptr)
            methcallbacks.lightController_setCloudActive(on.toInt());
        request->send(200);
    }
    else
        request->send(404);
}

void onGetSettings(AsyncWebServerRequest *request)
{
    request->send(200, "text/json", methcallbacks.getFanControllerSettings());
}

MyWebServerMethodCallbacks *MyWebServer_getCallbacksStruct()
{
    return &methcallbacks;
}

#ifdef USE_SDCARD
void getFile(AsyncWebServerRequest *request)
{
    String year = request->arg("year");
    String month = request->arg("month");
    String day = request->arg("day");
    String hour = request->arg("hour");
    String ret = "/" + year + "/" + month + "/" + day + "/" + hour + ".csv";
    if (methcallbacks.fileController_read != nullptr)
        request->send(200, "text/csv", methcallbacks.fileController_read(ret));
}
#endif

static const size_t MAX_SPIFS_UPLOAD = 1024 * 256; // 256 KiB – adjust to your partition size

// Helper: write the uploaded data directly into the SPIFFS partition
static void writeSpiffsBin(const uint8_t *buf, size_t len)
{
    const esp_partition_t *part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
        NULL); // first SPIFFS partition

    if (!part)
    {
        log_e("SPIFFS partition not found");
        return;
    }

    size_t written = 0;
    esp_err_t err = esp_partition_write(part, 0, buf, len);
    if (err != ESP_OK)
    {
        log_e("esp_partition_write failed: %d", err);
    }
    else
    {
        written = len;
        log_i("Written %u bytes to SPIFFS partition", (uint32_t)written);
    }
}

void MyWebServer_setup()
{
    SPIFFS.begin();
    server = new AsyncWebServer(http_port);
    server->on("/cmd", HTTP_GET, onCmd);
    server->on("/settings", HTTP_GET, onGetSettings);
#ifdef USE_SDCARD
    server->on("/data", HTTP_GET, getFile);
#endif
    static const esp_partition_t *spi_part = nullptr;

    server->on("/flashspiffs", HTTP_POST,
               /* request‑start handler (optional) */
               [](AsyncWebServerRequest *request)
               {
            // Nothing special needed here – just confirm the method
            if (!request->hasHeader("Content-Type")) {
                request->send(400, "text/plain", "Missing Content-Type");
                return;
            } },
               /* upload‑handler: (req, filename, index, data, len, final) */
               [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
               {
            // Find the SPIFFS partition once per upload
                   if (index == 0) {
                       spi_part = esp_partition_find_first(
                           ESP_PARTITION_TYPE_DATA,
                           ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
                           NULL);
                       if (!spi_part) {
                           request->send(500, "text/plain",
                                          "SPIFFS partition not found");
                           return;
                       }
                   }

                   // Keep track of the current byte offset
                   static size_t offset = 0;   // <‑‑ added

                   // Write the current chunk at the correct offset
                   esp_err_t err = esp_partition_write(spi_part,
                                                       offset,
                                                       data,
                                                       len);
                   if (err != ESP_OK) {
                       request->send(500, "text/plain",
                                     "Failed to write to SPIFFS");
                       return;
                   }

                   // Update the offset for the next chunk
                   offset += len;   // <‑‑ added

                   // When the final chunk arrives we finish the upload
                   if (final) {
                       log_i("File %s uploaded (%zu bytes)",
                             filename.c_str(),
                             offset);
                       request->send(200,
                                     "text/plain",
                                     "File uploaded successfully. Rebooting in 3 seconds...");
                       delay(3000);
                       ESP.restart();
                   } });

    static const esp_partition_t *fw_part = nullptr;
    static esp_ota_handle_t ota_handle = 0;

    server->on("/flashfirmware", HTTP_POST,
               /* request‑start handler (optional) */
               [](AsyncWebServerRequest *request)
               {
                   if (!request->hasHeader("Content-Type")) {
                       request->send(400, "text/plain", "Missing Content-Type");
                       return;
                   } },
               /* upload‑handler: (req, filename, index, data, len, final) */
               [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
               {
                   // Static OTA handle that survives across chunks
                   static esp_ota_handle_t ota_handle = 0;
                   static const esp_partition_t *fw_part_const = nullptr;

                   if (index == 0) {                 // first chunk – start new OTA
                       ota_handle = 0;              // reset any stale handle

                       fw_part_const = esp_partition_find_first(
                           ESP_PARTITION_TYPE_APP,
                           ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);

                       if (!fw_part_const) {
                           request->send(500, "text/plain",
                                         "Firmware partition not found");
                           return;
                       }

                       esp_err_t err = esp_ota_begin((esp_partition_t *)fw_part_const,
                                                     OTA_WITH_SEQUENTIAL_WRITES,
                                                     &ota_handle);
                       if (err != ESP_OK) {
                           request->send(500, "text/plain",
                                         "OTA begin failed: " + String(err));
                           return;
                       }
                   }

                   // Ensure we have a valid handle before writing
                   if (ota_handle == 0) {
                       request->send(500, "text/plain", "Invalid OTA handle");
                       return;
                   }

                   esp_err_t err = esp_ota_write(ota_handle, data, len);
                   if (err != ESP_OK) {
                       request->send(500, "text/plain",
                                     "OTA write failed: " + String(err));
                       return;
                   }

                   if (final) {                     // last chunk
                       err = esp_ota_end(ota_handle);
                       if (err == ESP_OK) {
                           log_i("Firmware %s uploaded (%zu bytes)",
                                 filename.c_str(), index + len);

                           request->send(200,
                                         "text/plain",
                                         "Firmware uploaded successfully. Rebooting in 3 seconds...");
                           delay(3000);                // give client time to read response
                           ESP.restart();              // reboot into new firmware
                       } else {
                           request->send(500, "text/plain",
                                         "OTA end failed: " + String(err));
                       }
                   } });

    server->serveStatic("/", SPIFFS, "/angular-www/").setDefaultFile("index.html");
    server->serveStatic("/", SD, "/");
    // server->serveStatic("/", SPIFFS, "/www/");
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

bool MyWebServer_WsClientsConnected()
{
    return ws_clients > 0;
}
