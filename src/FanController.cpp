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
// in blowing fans need to run slower to compensate the resistance from the filter and keep a bit vacuum inside the tent
int filtercompensation = 5; //%

float (*getTemp)();
float (*getHumidity)();

u_int16_t getVoltageFromPercent(int maxvoltage, int minvoltage, int val)
{
    return minvoltage + (float)(maxvoltage - minvoltage) * ((float)val / 100);
}

void FanController_processAutoControl()
{
    if (getTemp() > targetTemperature || getHumidity() > targetHumidity)
    {
        if (autocontrolfanspeed + 2 <= 100)
            autocontrolfanspeed += 2;
        else
            log_i("max speed reached");
    }
    else if (getTemp() < targetTemperature || getHumidity() < targetHumidity)
    {
        if (autocontrolfanspeed - 2 >= 10)
            autocontrolfanspeed -= 2;
        else
            log_i("min speed reached");
    }
    voltage = getVoltageFromPercent(maxVoltage, minVoltage, autocontrolfanspeed);
    voltage1 = getVoltageFromPercent(maxVoltage1, minVoltage1, autocontrolfanspeed - filtercompensation);
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

void FanController_setTargetTempHum(int temp, int hum)
{
    targetTemperature = temp;
    targetHumidity = hum;
    preferences.begin(prefNamespace, false);
    preferences.putInt("tTemp", targetTemperature);
    preferences.putInt("tHum", targetHumidity);
    preferences.end();
}

void FanController_setAutoControl(bool enable)
{
    autocontrol = enable;
    preferences.begin(prefNamespace, false);
    preferences.putInt("autocontrol", autocontrol);
    preferences.end();
}

String FanController_getSettings()
{
    JSONVar myObject;
    myObject["fan0voltage"] = voltage;
    myObject["fan0min"] = minVoltage;
    myObject["fan0max"] = maxVoltage;
    myObject["fan1voltage"] = voltage1;
    myObject["fan1min"] = minVoltage1;
    myObject["fan1max"] = maxVoltage1;
    myObject["autocontrol"] = autocontrol;
    myObject["targetTemperature"] = targetTemperature;
    myObject["targetHumidity"] = targetHumidity;
    return JSON.stringify(myObject);
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
