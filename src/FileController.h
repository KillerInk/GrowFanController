#pragma once
#include "Arduino.h"

void FileController_setup();
void FileController_write(double temp, double hum, int fanspeed, int co2, int lightmv, double vpd);
void FileController_write();
String FileController_read(String name);