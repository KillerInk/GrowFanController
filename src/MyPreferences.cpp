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

double MyPreferences_getDouble(const char * preferencename, const char *key, double defaultval)
{
    double ret = 0;
    preferences.begin(preferencename);
    ret = preferences.getDouble(key,defaultval);
    preferences.end();
    return ret;
}

void MyPreferences_setDouble(const char * preferencename, const char *key, double defaultval)
{
    double ret = 0;
    preferences.begin(preferencename);
    preferences.putDouble(key,defaultval);
    preferences.end();
}

void MyPreferences_setBytes(const char *key, const void *value, int len)
{
    preferences.begin(prefN);
    preferences.putBytes(key, value,len);
    preferences.end();
}