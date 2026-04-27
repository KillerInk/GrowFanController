#pragma once
#include "Arduino.h"
#include <WiFi.h>

// Forward declaration
class AsyncWebServerRequest;

#define WIFI_DEFAULT_CONNECT_TIMEOUT 15
#define WIFI_AP_SSID_PREFIX "GrowControl-"
#define WIFI_AP_DEFAULT_PASSWORD ""
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_MAX_CLIENTS 4

typedef struct {
    char ssid[33];
    char password[65];
    bool connected;
    int rssi;
    bool apActive;
    char apSsid[33];
    char apIp[16];
    bool apFallbackEnabled;
    uint8_t connectTimeout;
    bool savedCredsExist;
} WifiStatus;

class WiFiManager {
public:
    void setup();
    void loop();
    
    bool isConnected();
    bool isApActive();
    const char* getApSsid();
    const char* getApIp();
    WifiStatus getStatus();
    
    // WiFi config page handlers
    String getWifiConfigPage();
    void handleWifiConfigPost(AsyncWebServerRequest *request);
    
    // Save credentials and reboot
    void saveCredentials(const char* ssid, const char* password);
    
    // Periodic WiFi retry (when in AP mode)
    void retryWifiConnection();

private:
    void loadSavedCredentials();
    void saveConfigToNvs();
    void startApMode();
    void stopApMode();
    void tryStaConnection();
    
    WifiStatus _status;
    uint32_t _lastConnectAttempt;
    uint32_t _lastApStart;
    uint32_t _lastWifiRetry;
    bool _apStarted;
    bool _wifiConnected;
    bool _waitingForConnect;
};

extern WiFiManager wifiManager;
