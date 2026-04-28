#pragma once
#include "Arduino.h"

#define NVS_WIFI_NS "WiFi"

void MyPreferences_setup();
void MyPreferences_setBytes(const char* key, const void* value, int len);
void Mypreferences_getBytes(const char* key, void * buf, int maxLen);
double MyPreferences_getDouble(const char * preference,const char* key,double defaultval);
void MyPreferences_setDouble(const char * preferencename, const char* key, double defaultval);

// WiFi credential helpers
void MyPreferences_setWifiCredentials(const char* ssid, const char* pass);
bool MyPreferences_getWifiCredentials(char* ssid, int ssidMax, char* pass, int passMax);
bool MyPreferences_getBool(const char* ns, const char* key, bool def);
void MyPreferences_setBool(const char* ns, const char* key, bool val);
uint8_t MyPreferences_getUChar(const char* ns, const char* key, uint8_t def);
void MyPreferences_setUChar(const char* ns, const char* key, uint8_t val);

// Timezone offset helpers (stored as int8_t, range -12 to +13)
int8_t MyPreferences_getTimeZoneOffset();
void MyPreferences_setTimeZoneOffset(int8_t offset);