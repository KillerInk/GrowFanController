#include "FanController.h"
#include "DFRobot_GP8403.h"
#include "config.h"
#include "Preferences.h"
#include "JSON.h"

//[    47][I][main.cpp:310] setup(): found address 56
//[    51][I][main.cpp:310] setup(): found address 83
//[    53][I][main.cpp:310] setup(): found address 95
DFRobot_GP8403 dac(&Wire, i2c_pwn_addr);
uint16_t voltage = 0;
uint16_t voltage1 = 0;

Preferences preferences;
int minVoltage = 800;    // hz
int maxVoltage = 1100;   // hz
int minVoltage1 = 0;     // hz
int maxVoltage1 = 10000; // hz
const char *prefNamespace = "Voltage";

int targetTemperature = 26;
int targetHumidity = 60;
bool autocontrol = false;
// tracked fanspeed in autocontrol in %
int autocontrolfanspeed = 50; // %
int minspeed = 0;
int maxspeed = 100;
// in blowing fans need to run slower to compensate the resistance from the filter and keep a bit vacuum inside the tent
int filtercompensation = 5; //%

float (*getTemp)();
float (*getHumidity)();
float (*getAvgTemp)();
float (*getAvgHumidity)();

u_int16_t getVoltageFromPercent(int maxvoltage, int minvoltage, int val)
{
    return minvoltage + (float)(maxvoltage - minvoltage) * ((float)val / 100);
}

long nextTick;
float lastTemp;
void FanController_processAutoControl()
{
    float tmp = getTemp();
    float hm = getHumidity();
    float atmp = getAvgTemp();
    float ahm = getAvgHumidity();
    if (atmp > targetTemperature || ahm > targetHumidity)
    {
        autocontrolfanspeed++;
    }
    else if (atmp < targetTemperature || ahm < targetHumidity)
    {
        if(atmp < (targetTemperature - 1))
            autocontrolfanspeed--;
        else
        {
            if (nextTick < millis())
            {
                if (lastTemp > atmp)
                    autocontrolfanspeed--;
                else if (lastTemp < atmp)
                    autocontrolfanspeed++;
                lastTemp = atmp;
                nextTick = millis() + 5 * 1000;
            }
        }
    }
    if (autocontrolfanspeed > maxspeed)
        autocontrolfanspeed = maxspeed;

    if (autocontrolfanspeed < minspeed)
        autocontrolfanspeed = minspeed;
    voltage = getVoltageFromPercent(maxVoltage, minVoltage, autocontrolfanspeed);

    int fan2speed = autocontrolfanspeed - filtercompensation;
    if (fan2speed < 0)
        fan2speed = 0;
    if (fan2speed > 100)
        fan2speed = 100;
    voltage1 = getVoltageFromPercent(maxVoltage1, minVoltage1, fan2speed);
    dac.setDACOutVoltage(voltage, 0);
    dac.setDACOutVoltage(voltage1, 1);
    log_i("autocontrol set speed to: %i", autocontrolfanspeed);
}

bool FanController_isAutoControl()
{
    return autocontrol;
}

int FanController_getAutoSpeed()
{
    return autocontrolfanspeed;
}

void FanController_setVoltage(int id, int min, int max)
{
    preferences.begin(prefNamespace, false);
    if (id == 0)
    {
        minVoltage = min;
        maxVoltage = max;
        preferences.putInt("minv0", minVoltage);
        preferences.putInt("maxv0", maxVoltage);
    }
    else if (id == 1)
    {
        minVoltage1 = min;
        maxVoltage1 = max;
        preferences.putInt("minv1", minVoltage1);
        preferences.putInt("maxv1", maxVoltage1);
    }
    preferences.end();
}

void FanController_setMinMaxFanSpeed(int min, int max)
{
    minspeed = min;
    maxspeed = max;
    preferences.begin(prefNamespace, false);
    preferences.putInt("mins", minspeed);
    preferences.putInt("maxs", maxspeed);
    preferences.putInt("fc", filtercompensation);
    preferences.end();
}

void FanController_setTargetTempHumSpeedDif(int temp, int hum, int speeddif)
{
    targetTemperature = temp;
    targetHumidity = hum;
    filtercompensation = speeddif;
    preferences.begin(prefNamespace, false);
    preferences.putInt("tTemp", targetTemperature);
    preferences.putInt("tHum", targetHumidity);
    preferences.putInt("fc", filtercompensation);
    preferences.end();
}

void FanController_setAutoControl(bool enable)
{
    autocontrol = enable;
    preferences.begin(prefNamespace, false);
    preferences.putInt("autocontrol", autocontrol);
    preferences.end();
}

int getVoltage()
{
    return voltage;
}

int getVoltage1()
{
    return voltage1;
}

int getMaxVoltage()
{
    return maxVoltage;
}

int getMaxVoltage1()
{
    return maxVoltage1;
}

int getMinVoltage()
{
    return minVoltage;
}

int getMinVoltage1()
{
    return minVoltage1;
}

int getTargetHumidity()
{
    return targetHumidity;
}

int getTargetTemperature()
{
    return targetTemperature;
}

int getSpeedDifference()
{
    return filtercompensation;
}

int FanController_getMinSpeed()
{
    return minspeed;
}

int FanController_getMaxSpeed()
{
    return maxspeed;
}

void FanController_applyspeed(int volt, int id, int val)
{
    if (val > 0)
    {
        if (id == 0)
        {
            u_int16_t s = getVoltageFromPercent(maxVoltage, minVoltage, val);
            volt = s;
            voltage = volt;
        }
        else if (id == 1)
        {
            u_int16_t s = getVoltageFromPercent(maxVoltage1, minVoltage1, val);
            volt = s;
            voltage1 = volt;
        }
    }
    else
        volt = 0;
    log_i("set id:%i voltage to %i", id, volt);
    dac.setDACOutVoltage(volt, id);
}

void FanController_setup()
{
    preferences.begin(prefNamespace, false);
    maxVoltage = preferences.getInt("maxv0", maxVoltage);
    minVoltage = preferences.getInt("minv0", minVoltage);
    maxVoltage1 = preferences.getInt("maxv1", maxVoltage1);
    minVoltage1 = preferences.getInt("minv1", minVoltage1);
    targetHumidity = preferences.getInt("tHum", targetHumidity);
    targetTemperature = preferences.getInt("tTemp", targetTemperature);
    autocontrol = preferences.getInt("autocontrol", autocontrol);
    filtercompensation = preferences.getInt("fc", filtercompensation);
    minspeed = preferences.getInt("mins", minspeed);
    maxspeed = preferences.getInt("maxs", maxspeed);
    preferences.end();
    log_i("dac avail:%i", dac.begin());
    // Set DAC output range
    dac.setDACOutRange(dac.eOutputRange10V);
}

void FanController_setHumidityAndTempFunctions(float func(), float func2())
{
    getTemp = func2;
    getHumidity = func;
}

void FanController_setAvgHumidityAndTempFunctions(float func(), float func2())
{
    getAvgTemp = func2;
    getAvgHumidity = func;
}
