#include "MyPreferences.h"
#include "Preferences.h"
Preferences preferences;
const char *prefN= "Voltage";

void MyPreferences_setup()
{
}

void Mypreferences_getBytes(const char *key, void *buf, int maxLen)
{
    preferences.begin(prefN);
    preferences.getBytes(key, buf, maxLen);
    preferences.end();
}

void MyPreferences_setBytes(const char *key, const void *value, int len)
{
    preferences.begin(prefN);
    preferences.putBytes(key, value,len);
    preferences.end();
}