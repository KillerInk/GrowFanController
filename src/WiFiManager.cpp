#include "WiFiManager.h"
#include "MyPreferences.h"
#include "ESPAsyncWebServer.h"
#include "esp_partition.h"
#include "Preferences.h"
#include <esp_mac.h>

WiFiManager wifiManager;

void WiFiManager::setup() {
    memset(&_status, 0, sizeof(_status));
    _apStarted = false;
    _wifiConnected = false;
    _waitingForConnect = false;
    _lastConnectAttempt = 0;
    _lastApStart = 0;
    _lastWifiRetry = 0;

    // Load saved credentials from NVS
    loadSavedCredentials();

    // Load AP config from NVS
    Preferences apPrefs;
    apPrefs.begin(NVS_WIFI_NS, true); // read-only
    _status.apFallbackEnabled = apPrefs.getBool("ap_en", true);
    apPrefs.end();

    // Try to connect to saved WiFi
    if (_status.savedCredsExist && strlen(_status.ssid) > 0) {
        tryStaConnection();
    } else {
        // No saved credentials — start AP immediately
        if (_status.apFallbackEnabled) {
            startApMode();
        }
    }
}

void WiFiManager::loop() {
    // Check if WiFi connection succeeded
    if (_waitingForConnect && WiFi.status() == WL_CONNECTED) {
        _wifiConnected = true;
        _waitingForConnect = false;
        _status.connected = true;
        _status.ssid[0] = '\0';
        strncpy(_status.ssid, WiFi.SSID().c_str(), sizeof(_status.ssid) - 1);
        _status.rssi = WiFi.RSSI();
        log_i("WiFi connected to %s (RSSI: %d dBm)", _status.ssid, _status.rssi);
        
        // Stop AP if we connected (keep APSTA mode but deactivate softAP)
        if (_apStarted) {
            stopApMode();
        }
        return;
    }

    // Check for connection timeout
    if (_waitingForConnect && millis() - _lastConnectAttempt > _status.connectTimeout * 1000UL) {
        _waitingForConnect = false;
        log_w("WiFi connection timed out after %d seconds", _status.connectTimeout);
        
        if (_status.apFallbackEnabled) {
            startApMode();
        }
        return;
    }

    // Periodic WiFi retry when in AP mode (every 60 seconds)
    if (_status.apActive && !_wifiConnected && millis() - _lastWifiRetry > 60000UL) {
        _lastWifiRetry = millis();
        retryWifiConnection();
    }
}

void WiFiManager::tryStaConnection() {
    // Validate SSID before attempting connection
    if (_status.ssid[0] == '\0' || strlen(_status.ssid) > 32) {
        log_w("Cannot start STA: invalid SSID");
        return;
    }
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false);
    WiFi.begin(_status.ssid, _status.password);
    _waitingForConnect = true;
    _lastConnectAttempt = millis();
    log_i("Attempting WiFi connection to '%s' (timeout: %ds)...", _status.ssid, _status.connectTimeout);
}

void WiFiManager::retryWifiConnection() {
    if (_wifiConnected || _waitingForConnect) return;
    
    // Validate SSID before attempting connection
    if (_status.ssid[0] == '\0' || strlen(_status.ssid) > 32) {
        log_w("Skipping WiFi retry: invalid SSID");
        return;
    }
    
    // Try to connect in the background while keeping AP active
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(_status.ssid, _status.password);
    _waitingForConnect = true;
    _lastConnectAttempt = millis();
    log_i("Retrying WiFi connection to '%s' in background...", _status.ssid);
}

void WiFiManager::startApMode() {
    if (_apStarted) return;

    WiFi.mode(WIFI_AP_STA);
    
    // Generate AP SSID from chip ID
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    snprintf(this->_status.apSsid, sizeof(this->_status.apSsid), "%s%02X%02X",
             WIFI_AP_SSID_PREFIX, mac[4], mac[5]);
    
    // Load AP password from NVS
    bool apOpen = MyPreferences_getBool(NVS_WIFI_NS, "ap_pw", true);
    String apPass = apOpen ? String(WIFI_AP_DEFAULT_PASSWORD) : String("");
    
    WiFi.softAP(this->_status.apSsid, apPass.c_str(), WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CLIENTS);
    
    IPAddress ip = WiFi.softAPIP();
    snprintf(this->_status.apIp, sizeof(this->_status.apIp), "%d.%d.%d.%d",
             ip[0], ip[1], ip[2], ip[3]);
    
    this->_status.apActive = true;
    _apStarted = true;
    _lastApStart = millis();
    
    log_i("AP started: SSID='%s', IP=%s", this->_status.apSsid, this->_status.apIp);
}

void WiFiManager::stopApMode() {
    if (!_apStarted) return;
    
    WiFi.softAPdisconnect(true);
    this->_status.apActive = false;
    _apStarted = false;
    log_i("AP stopped");
}

void WiFiManager::loadSavedCredentials() {
    char ssid[33] = {0};
    char pass[65] = {0};
    
    _status.savedCredsExist = MyPreferences_getWifiCredentials(ssid, sizeof(ssid), pass, sizeof(pass));
    
    log_i("loadSavedCredentials: saved=%d, ssid='%s' (len=%d), pass_len=%d",
          _status.savedCredsExist, ssid, strlen(ssid), strlen(pass));
    
    if (_status.savedCredsExist) {
        strncpy(_status.ssid, ssid, sizeof(_status.ssid) - 1);
        _status.ssid[sizeof(_status.ssid) - 1] = '\0';
        strncpy(_status.password, pass, sizeof(_status.password) - 1);
        _status.password[sizeof(_status.password) - 1] = '\0';
    } else {
        _status.ssid[0] = '\0';
        _status.password[0] = '\0';
    }
}

void WiFiManager::saveConfigToNvs() {
    Preferences prefs;
    prefs.begin(NVS_WIFI_NS, false); // read-write
    log_i("NVS: writing ssid='%s' (%d chars), pass='%s' (%d chars), ap_en=%d, to=%d",
          _status.ssid, (int)strlen(_status.ssid), _status.password, (int)strlen(_status.password),
          _status.apFallbackEnabled, _status.connectTimeout);
    prefs.putString("ssid", _status.ssid);
    prefs.putString("pass", _status.password);
    prefs.putBool("ap_en", _status.apFallbackEnabled);
    prefs.putUChar("to", _status.connectTimeout);
    
    // Verify write by reading back
    String verifySsid = prefs.getString("ssid", "VERIFY_FAIL");
    log_i("NVS: verify read ssid='%s'", verifySsid.c_str());
    
    prefs.end();
    log_i("NVS: write complete");
}

void WiFiManager::saveCredentials(const char* ssid, const char* password) {
    strncpy(this->_status.ssid, ssid, sizeof(this->_status.ssid) - 1);
    this->_status.ssid[sizeof(this->_status.ssid) - 1] = '\0';
    strncpy(this->_status.password, password, sizeof(this->_status.password) - 1);
    this->_status.password[sizeof(this->_status.password) - 1] = '\0';
    this->_status.savedCredsExist = true;
    
    saveConfigToNvs();
    
    log_i("Credentials saved (ssid='%s'). Rebooting to connect...", _status.ssid);
    delay(100);
    ESP.restart();
}

bool WiFiManager::isConnected() {
    return _wifiConnected && WiFi.status() == WL_CONNECTED;
}

bool WiFiManager::isApActive() {
    return this->_status.apActive;
}

const char* WiFiManager::getApSsid() {
    return this->_status.apSsid;
}

const char* WiFiManager::getApIp() {
    return this->_status.apIp;
}

WifiStatus WiFiManager::getStatus() {
    _status.connected = (_wifiConnected && WiFi.status() == WL_CONNECTED);
    if (_status.connected) {
        _status.rssi = WiFi.RSSI();
        strncpy(_status.ssid, WiFi.SSID().c_str(), sizeof(_status.ssid) - 1);
        _status.ssid[sizeof(_status.ssid) - 1] = '\0';
    }
    return _status;
}

String WiFiManager::getWifiConfigPage() {
    String html = R"rawliteral(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>WiFi Configuration - GrowControl</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f0f2f5; min-height: 100vh; display: flex; align-items: center; justify-content: center; }
.card { background: #fff; border-radius: 12px; box-shadow: 0 2px 12px rgba(0,0,0,0.1); padding: 32px; width: 100%; max-width: 420px; }
h1 { font-size: 1.5rem; color: #1a1a2e; margin-bottom: 8px; }
.subtitle { color: #666; font-size: 0.9rem; margin-bottom: 24px; }
.form-group { margin-bottom: 16px; }
label { display: block; font-size: 0.85rem; font-weight: 600; color: #333; margin-bottom: 4px; }
input[type="text"], input[type="password"] { width: 100%; padding: 10px 12px; border: 1px solid #ddd; border-radius: 8px; font-size: 1rem; transition: border-color 0.2s; }
input:focus { outline: none; border-color: #4a90d9; box-shadow: 0 0 0 3px rgba(74,144,217,0.1); }
.btn { width: 100%; padding: 12px; background: #4a90d9; color: #fff; border: none; border-radius: 8px; font-size: 1rem; font-weight: 600; cursor: pointer; transition: background 0.2s; margin-top: 8px; }
.btn:hover { background: #357abd; }
.status { padding: 12px; border-radius: 8px; margin-bottom: 20px; font-size: 0.9rem; }
.status.connected { background: #e8f5e9; color: #2e7d32; border: 1px solid #c8e6c9; }
.status.disconnected { background: #fff3e0; color: #e65100; border: 1px solid #ffe0b2; }
.toggle-row { display: flex; align-items: center; justify-content: space-between; padding: 8px 0; }
.toggle-row label { margin: 0; }
.toggle { position: relative; width: 44px; height: 24px; }
.toggle input { opacity: 0; width: 0; height: 0; }
.toggle .slider { position: absolute; inset: 0; background: #ccc; border-radius: 24px; cursor: pointer; transition: 0.3s; }
.toggle .slider:before { content: ""; position: absolute; height: 18px; width: 18px; left: 3px; bottom: 3px; background: #fff; border-radius: 50%; transition: 0.3s; }
.toggle input:checked + .slider { background: #4a90d9; }
.toggle input:checked + .slider:before { transform: translateX(20px); }
.info { font-size: 0.8rem; color: #888; margin-top: 16px; text-align: center; }
</style>
</head>
<body>
<div class="card">
    <h1>WiFi Configuration</h1>
    <p class="subtitle">Configure WiFi credentials for GrowControl device</p>
    <div id="status" class="status connected">Connected to: <span id="currentSsid"></span></div>
    <form id="wifiForm" action="/wifi-config" method="POST">
        <div class="form-group">
            <label for="ssid">WiFi SSID</label>
            <input type="text" id="ssid" name="ssid" placeholder="Enter WiFi network name" required>
        </div>
        <div class="form-group">
            <label for="password">Password</label>
            <input type="password" id="password" name="password" placeholder="Enter WiFi password">
        </div>
        <div class="toggle-row">
            <label>AP Fallback</label>
            <label class="toggle">
                <input type="checkbox" id="apFallback" name="ap_fallback" checked>
                <span class="slider"></span>
            </label>
        </div>
        <div class="form-group" style="margin-top: 12px;">
            <label for="timeout">Connection Timeout (seconds)</label>
            <input type="text" id="timeout" name="timeout" value="15" pattern="[0-9]+" title="1-60 seconds">
        </div>
        <button type="submit" class="btn">Save & Reboot</button>
    </form>
    <p class="info">Device will reboot to apply new WiFi settings</p>
</div>
<script>
// Populate current status
(function() {
    var statusEl = document.getElementById('status');
    var ssidEl = document.getElementById('currentSsid');
    // Check if we're on AP (no WiFi connected)
    var isAp = window.location.search.includes('mode=ap');
    if (isAp) {
        statusEl.className = 'status disconnected';
        ssidEl.textContent = 'No WiFi connected — configure below';
    }
})();
</script>
</body>
</html>)rawliteral";
    return html;
}

void WiFiManager::handleWifiConfigPost(AsyncWebServerRequest *request) {
    String ssid = request->arg("ssid");
    String password = request->arg("password");
    String apFallback = request->arg("ap_fallback");
    String timeout = request->arg("timeout");
    
    log_i("WiFi config received: ssid='%s' (len=%d), ap_fallback=%s, timeout=%s", ssid.c_str(), ssid.length(), apFallback.c_str(), timeout.c_str());
    
    // Validate SSID length
    if (ssid.length() < 1 || ssid.length() > 32) {
        request->send(400, "text/plain", "Invalid SSID length (1-32 characters)");
        return;
    }
    
    // Validate password length
    if (password.length() > 0 && (password.length() < 8 || password.length() > 64)) {
        request->send(400, "text/plain", "Invalid password length (8-64 characters or empty)");
        return;
    }
    
    // Save AP config
    MyPreferences_setBool(NVS_WIFI_NS, "ap_en", apFallback == "on" || apFallback == "1");
    if (timeout.toInt() > 0 && timeout.toInt() <= 60) {
        MyPreferences_setUChar(NVS_WIFI_NS, "to", (uint8_t)timeout.toInt());
    }
    
    // Save credentials and reboot
    saveCredentials(ssid.c_str(), password.c_str());
}
