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

    preferences.begin(NVS_WIFI_NS, false);
    preferences.putString("ssid", ssid);
    preferences.putString("pass", pass);
    preferences.end();
}

bool MyPreferences_getWifiCredentials(char* ssid, int ssidMax, char* pass, int passMax)
{

    preferences.begin(NVS_WIFI_NS, true); // read-only
    String s = preferences.getString("ssid", "");
    String p = preferences.getString("pass", "");
    preferences.end();
    
    if (s.length() == 0) return false;
    
    s.toCharArray(ssid, ssidMax);
    p.toCharArray(pass, passMax);
    return true;
}

bool MyPreferences_getBool(const char* ns, const char* key, bool def)
{
    preferences.begin(ns, true);
    bool val = preferences.getBool(key, def);
    preferences.end();
    return val;
}

void MyPreferences_setBool(const char* ns, const char* key, bool val)
{

    preferences.begin(ns, false);
    preferences.putBool(key, val);
    preferences.end();
}

uint8_t MyPreferences_getUChar(const char* ns, const char* key, uint8_t def)
{

    preferences.begin(ns, true);
    uint8_t val = preferences.getUChar(key, def);
    preferences.end();
    return val;
}

void MyPreferences_setUChar(const char* ns, const char* key, uint8_t val)
{

    preferences.begin(ns, false);
    preferences.putUChar(key, val);
    preferences.end();
}

// Timezone offset helpers (stored as uint8_t, actual value = val - 64, range -64..+63 -> maps to -12..+13 hours)
static const int8_t TZ_DEFAULT_OFFSET = time_zone_hour_utc_offset;
static const uint8_t TZ_NVS_BASE = 64; // offset for NVS storage: 0 maps to -64, 64 maps to 0, 79 maps to +15

int8_t MyPreferences_getTimeZoneOffset()
{

    preferences.begin("TimeZone", true);
    uint8_t val = preferences.getUChar("offset", TZ_NVS_BASE + TZ_DEFAULT_OFFSET);
    preferences.end();
    return (int8_t)(val - TZ_NVS_BASE);
}

void MyPreferences_setTimeZoneOffset(int8_t offset)
{
    preferences.begin("TimeZone", false);
    preferences.putUChar("offset", TZ_NVS_BASE + (uint8_t)offset);
    preferences.end();
}