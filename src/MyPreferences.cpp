#include "MyPreferences.h"
#include "Preferences.h"

Preferences preferences;
const char *prefN = "Voltage";

void MyPreferences_setup()
{
}

void MyPreferences_setBytes(const char* key, const void* value, int len)
{
    preferences.begin(prefN);
    preferences.putBytes(key, value, len);
    preferences.end();
}

void Mypreferences_getBytes(const char* key, void *buf, int maxLen)
{
    preferences.begin(prefN);
    preferences.getBytes(key, buf, maxLen);
    preferences.end();
}

double MyPreferences_getDouble(const char *preferencename, const char *key, double defaultval)
{
    double ret = 0;
    preferences.begin(preferencename);
    ret = preferences.getDouble(key, defaultval);
    preferences.end();
    return ret;
}

void MyPreferences_setDouble(const char *preferencename, const char *key, double defaultval)
{
    preferences.begin(preferencename);
    preferences.putDouble(key, defaultval);
    preferences.end();
}

// ---------- WiFi credential helpers ----------

void MyPreferences_setWifiCredentials(const char* ssid, const char* pass)
{
    Preferences prefs;
    prefs.begin(NVS_WIFI_NS, false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
}

bool MyPreferences_getWifiCredentials(char* ssid, int ssidMax, char* pass, int passMax)
{
    Preferences prefs;
    prefs.begin(NVS_WIFI_NS, true); // read-only
    String s = prefs.getString("ssid", "");
    String p = prefs.getString("pass", "");
    prefs.end();
    
    if (s.length() == 0) return false;
    
    s.toCharArray(ssid, ssidMax);
    p.toCharArray(pass, passMax);
    return true;
}

bool MyPreferences_getBool(const char* ns, const char* key, bool def)
{
    Preferences prefs;
    prefs.begin(ns, true);
    bool val = prefs.getBool(key, def);
    prefs.end();
    return val;
}

void MyPreferences_setBool(const char* ns, const char* key, bool val)
{
    Preferences prefs;
    prefs.begin(ns, false);
    prefs.putBool(key, val);
    prefs.end();
}

uint8_t MyPreferences_getUChar(const char* ns, const char* key, uint8_t def)
{
    Preferences prefs;
    prefs.begin(ns, true);
    uint8_t val = prefs.getUChar(key, def);
    prefs.end();
    return val;
}

void MyPreferences_setUChar(const char* ns, const char* key, uint8_t val)
{
    Preferences prefs;
    prefs.begin(ns, false);
    prefs.putUChar(key, val);
    prefs.end();
}