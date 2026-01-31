#pragma once

#define VOLTAGE_MIN 0
#define VOLTAGE_MAX 10000

struct Voltage
{
    int min = 0;
    int max = 1200;
    int voltage = 0;
};

static int getVoltageFromPercent(int maxvoltage, int minvoltage, double val)
{
    return static_cast<int>(minvoltage + (maxvoltage - minvoltage) * val / 100);
}
