#pragma once
#include "Arduino.h"
#include "ESPAsyncWebServer.h"

struct MyWebServerMethodCallbacks
{
    void (*applyspeed_listner)(int id, int val);
    void (*voltagechanged_listner)(int id, int min, int max);
    void (*targettemphum_listner)(int tmp, int hum, int speed);
    void (*autocontrol_listner)(bool enable);
    String (*getFanControllerSettings)();
#ifdef GOVEE_BTH5179
    void (*readgovee_listner)(bool enable);
#endif
    void (*setTempHumDif)(double temp, double hum);
    void (*setMinMaxSpeed)(int min, int max);
    void (*fancoltroller_nightmodecallback)(int onhour, int onmin, int offhour, int offmin, int maxspeed);
    void (*fancoltroller_nightmodeactivcecallback)(bool active);
    void (*lightController_setVoltageLimits)(int min, int max);
    void (*lightController_setTimes)(int onhour, int onmin, int offhour, int offmin, int risehour, int risemin, int sethour, int setmin, bool riseenable, bool setenable);
    void (*lightController_setLight)(int mv);
    void (*lightController_setAuto)(bool automode);
    void (*lightController_setPercentLimits)(int min, int max);
    void (*lightController_setCloudActive)(bool automode);
    void (*lightController_setCloudValues)(int min, int max, int cycleduration);
    // Lifecycle callbacks (added 2026-04-10)
    void (*lightController_setLifecycleEnabled)(bool enabled);
    void (*lightController_setLifecycleStage)(int stage);
    void (*lightController_resetLifecycle)(void);
    String (*lightController_getLifecycleState)(void);
    void (*lightController_setPanelPPFD)(float ppfd);
#ifdef USE_SDCARD
    String (*fileController_read)(String name);
#endif
    // WiFi config
    String (*wifiConfigGet)();
    void (*wifiConfigPost)(AsyncWebServerRequest *request);
    
    // Timezone config
    int (*getTimeZoneOffset)();
    void (*setTimeZoneOffset)(int offset);
};

MyWebServerMethodCallbacks *MyWebServer_getCallbacksStruct();
void MyWebServer_setup();
void MyWebServer_sendSocketMsg(String msg);
bool MyWebServer_WsClientsConnected();
