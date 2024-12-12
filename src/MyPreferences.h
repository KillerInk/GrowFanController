#pragma once
#include "Arduino.h"

void MyPreferences_setup();
void MyPreferences_setBytes(const char* key, const void* value, int len);
void Mypreferences_getBytes(const char* key, void * buf, int maxLen);
double MyPreferences_getDouble(const char * preference,const char* key,double defaultval);
void MyPreferences_setDouble(const char * preferencename, const char *key, double defaultval);