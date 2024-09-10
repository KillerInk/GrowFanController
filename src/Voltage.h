#pragma once

#define VOLTAGE_MIN 0
#define VOLTAGE_MAX 10000

struct Voltage
{
    int min = 0;
    int max = 1200;
    int voltage = 0;
};

static int getVoltageFromPercent(int maxvoltage, int minvoltage, float val)
{
    return (float)minvoltage + (float)(maxvoltage - minvoltage) * ((float)val / 100);
}
