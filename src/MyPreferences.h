#pragma once
#include "Arduino.h"

void MyPreferences_setup();
void MyPreferences_setBytes(const char* key, const void* value, int len);
void Mypreferences_getBytes(const char* key, void * buf, int maxLen);