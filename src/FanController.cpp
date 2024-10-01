#include "FanController.h"
#include "DFRobot_GP8403.h"
#include "config.h"
#include "MyPreferences.h"
#include "JSON.h"

//[    47][I][main.cpp:310] setup(): found address 56
//[    51][I][main.cpp:310] setup(): found address 83
//[    53][I][main.cpp:310] setup(): found address 95
DFRobot_GP8403 dac(&Wire, i2c_pwn_addr);

FanControllerValues fancontrollerValues;

double (*getTemp)();
double (*getHumidity)();
double (*getAvgTemp)();
double (*getAvgHumidity)();

long nextTick;
double lastTemp;
void FanController_processAutoControl()
{
    double atmp = getAvgTemp();
    double ahm = getAvgHumidity();
    if (atmp > fancontrollerValues.targetTemperature || ahm > fancontrollerValues.targetHumidity)
    {
        fancontrollerValues.autocontrolfanspeed++;
    }
    else if (atmp < fancontrollerValues.targetTemperature || ahm < fancontrollerValues.targetHumidity)
    {
        if (atmp < (fancontrollerValues.targetTemperature - 1))
            fancontrollerValues.autocontrolfanspeed--;
        else
        {
            if (lastTemp > atmp)
                fancontrollerValues.autocontrolfanspeed--;
            else if (lastTemp < atmp)
                fancontrollerValues.autocontrolfanspeed++;
            lastTemp = atmp;
        }
    }
    if (fancontrollerValues.autocontrolfanspeed > fancontrollerValues.maxspeed)
        fancontrollerValues.autocontrolfanspeed = fancontrollerValues.maxspeed;
    if (fancontrollerValues.nightmodeActive && fancontrollerValues.autocontrolfanspeed > fancontrollerValues.nightmodeMaxSpeed)
        fancontrollerValues.autocontrolfanspeed = fancontrollerValues.nightmodeMaxSpeed;

    if (fancontrollerValues.autocontrolfanspeed < fancontrollerValues.minspeed)
        fancontrollerValues.autocontrolfanspeed = fancontrollerValues.minspeed;
    fancontrollerValues.fan0Voltage.voltage = getVoltageFromPercent(fancontrollerValues.fan0Voltage.max, fancontrollerValues.fan0Voltage.min, fancontrollerValues.autocontrolfanspeed);

    int fan2speed = fancontrollerValues.autocontrolfanspeed - fancontrollerValues.filtercompensation;
    if (fan2speed < 0)
        fan2speed = 0;
    if (fan2speed > 100)
        fan2speed = 100;
    fancontrollerValues.fan1Voltage.voltage = getVoltageFromPercent(fancontrollerValues.fan1Voltage.max, fancontrollerValues.fan1Voltage.min, fan2speed);
    dac.setDACOutVoltage(fancontrollerValues.fan0Voltage.voltage, 0);
    dac.setDACOutVoltage(fancontrollerValues.fan1Voltage.voltage, 1);
    log_i("autocontrol set speed to: %i fan0 mv:%f fan1 mv:%f", fancontrollerValues.autocontrolfanspeed, fancontrollerValues.fan0Voltage.voltage, fancontrollerValues.fan1Voltage.voltage);
}

void FanController_setVoltage(int id, int min, int max)
{
    if (id == 0)
    {
        fancontrollerValues.fan0Voltage.min = min;
        fancontrollerValues.fan0Voltage.max = max;
    }
    else if (id == 1)
    {
        fancontrollerValues.fan1Voltage.min = min;
        fancontrollerValues.fan1Voltage.max = max;
    }
    MyPreferences_setBytes("conv", &fancontrollerValues, sizeof(FanControllerValues));
}

void FanController_setMinMaxFanSpeed(int min, int max)
{
    fancontrollerValues.minspeed = min;
    fancontrollerValues.maxspeed = max;
    MyPreferences_setBytes("conv", &fancontrollerValues, sizeof(FanControllerValues));
}

Voltage *FanController_getFan0()
{
    return &fancontrollerValues.fan0Voltage;
}

Voltage *FanController_getFan1()
{
    return &fancontrollerValues.fan1Voltage;
}

void FanController_loop()
{
    if (fancontrollerValues.nightmode)
    {
        tm time;
        getLocalTime(&time);
        fancontrollerValues.nightmodeActive = timeInRange(&fancontrollerValues.nightmodeOn, &fancontrollerValues.nightModeOff, time);
    }
}

void FanController_setNightMode(bool active)
{
    fancontrollerValues.nightmode = active;
    if (active)
    {
        tm time;
        getLocalTime(&time);
        fancontrollerValues.nightmodeActive = timeInRange(&fancontrollerValues.nightmodeOn, &fancontrollerValues.nightModeOff, time);
    }
    MyPreferences_setBytes("conv", &fancontrollerValues, sizeof(FanControllerValues));
}

void FanController_setNightModeValues(int onhour, int onmin, int offhour, int offmin, int maxspeed)
{
    fancontrollerValues.nightmodeOn.hour = onhour;
    fancontrollerValues.nightmodeOn.min = onmin;
    fancontrollerValues.nightModeOff.hour = offhour;
    fancontrollerValues.nightModeOff.min = offmin;
    fancontrollerValues.nightmodeMaxSpeed = maxspeed;
    MyPreferences_setBytes("conv", &fancontrollerValues, sizeof(FanControllerValues));
}

FanControllerValues *FanController_getValues()
{
    return &fancontrollerValues;
}

void FanController_setTargetTempHumSpeedDif(int temp, int hum, int speeddif)
{
    fancontrollerValues.targetTemperature = temp;
    fancontrollerValues.targetHumidity = hum;
    fancontrollerValues.filtercompensation = speeddif;
    MyPreferences_setBytes("conv", &fancontrollerValues, sizeof(FanControllerValues));
}

void FanController_setAutoControl(bool enable)
{
    fancontrollerValues.autocontrol = enable;
    MyPreferences_setBytes("conv", &fancontrollerValues, sizeof(FanControllerValues));
}

void FanController_applyspeed(int volt, int id, int val)
{
    if (val > 0)
    {
        if (id == 0)
        {
            u_int16_t s = getVoltageFromPercent(fancontrollerValues.fan0Voltage.max, fancontrollerValues.fan0Voltage.min, val);
            volt = s;
            fancontrollerValues.fan0Voltage.voltage = volt;
        }
        else if (id == 1)
        {
            u_int16_t s = getVoltageFromPercent(fancontrollerValues.fan1Voltage.max, fancontrollerValues.fan1Voltage.min, val);
            volt = s;
            fancontrollerValues.fan1Voltage.voltage = volt;
        }
    }
    else
        volt = 0;
    log_i("set id:%i voltage to %i", id, volt);
    dac.setDACOutVoltage(volt, id);
}

void FanController_setup()
{
    Mypreferences_getBytes("conv", &fancontrollerValues, sizeof(FanControllerValues));
    log_i("dac avail:%i", dac.begin());
    // Set DAC output range
    dac.setDACOutRange(dac.eOutputRange10V);
}

void FanController_setHumidityAndTempFunctions(double func(), double func2())
{
    getTemp = func2;
    getHumidity = func;
}

void FanController_setAvgHumidityAndTempFunctions(double func(), double func2())
{
    getAvgTemp = func2;
    getAvgHumidity = func;
}
