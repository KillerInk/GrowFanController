#pragma once

#define VOLTAGE_MIN 0
#define VOLTAGE_MAX 10000

struct Voltage
{
    int min;
    int max;
    int voltage;
};

static u_int16_t getVoltageFromPercent(int maxvoltage, int minvoltage, int val)
{
    return minvoltage + (float)(maxvoltage - minvoltage) * ((float)val / 100);
}