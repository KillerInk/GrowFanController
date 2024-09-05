#include "FanController.h"
#include "DFRobot_GP8403.h"
#include "config.h"
#include "MyPreferences.h"
#include "JSON.h"

//[    47][I][main.cpp:310] setup(): found address 56
//[    51][I][main.cpp:310] setup(): found address 83
//[    53][I][main.cpp:310] setup(): found address 95
DFRobot_GP8403 dac(&Wire, i2c_pwn_addr);
// mars hydro fan 560-630
// artic 120mm 560-1100;
Voltage fan0Voltage;
Voltage fan1Voltage;
FanControllerValues fancontrollerValues;

float (*getTemp)();
float (*getHumidity)();
float (*getAvgTemp)();
float (*getAvgHumidity)();

long nextTick;
float lastTemp;
void FanController_processAutoControl()
{
    float atmp = getAvgTemp();
    float ahm = getAvgHumidity();
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
            if (nextTick < millis())
            {
                if (lastTemp > atmp)
                    fancontrollerValues.autocontrolfanspeed--;
                else if (lastTemp < atmp)
                    fancontrollerValues.autocontrolfanspeed++;
                lastTemp = atmp;
                nextTick = millis() + 5 * 1000;
            }
        }
    }
    if (fancontrollerValues.autocontrolfanspeed > fancontrollerValues.maxspeed)
        fancontrollerValues.autocontrolfanspeed = fancontrollerValues.maxspeed;

    if (fancontrollerValues.autocontrolfanspeed < fancontrollerValues.minspeed)
        fancontrollerValues.autocontrolfanspeed = fancontrollerValues.minspeed;
    fan0Voltage.voltage = getVoltageFromPercent(fan0Voltage.max, fan0Voltage.min, fancontrollerValues.autocontrolfanspeed);

    int fan2speed = fancontrollerValues.autocontrolfanspeed - fancontrollerValues.filtercompensation;
    if (fan2speed < 0)
        fan2speed = 0;
    if (fan2speed > 100)
        fan2speed = 100;
    fan1Voltage.voltage = getVoltageFromPercent(fan1Voltage.max, fan1Voltage.min, fan2speed);
    dac.setDACOutVoltage(fan0Voltage.voltage, 0);
    dac.setDACOutVoltage(fan1Voltage.voltage, 1);
    log_i("autocontrol set speed to: %i", fancontrollerValues.autocontrolfanspeed);
}

void FanController_setVoltage(int id, int min, int max)
{
    if (id == 0)
    {
        fan0Voltage.min = min;
        fan0Voltage.max = max;
        MyPreferences_setBytes("fan0", &fan0Voltage, sizeof(Voltage));
    }
    else if (id == 1)
    {
        fan1Voltage.min = min;
        fan1Voltage.max = max;
        MyPreferences_setBytes("fan1", &fan1Voltage, sizeof(Voltage));
    }
}

void FanController_setMinMaxFanSpeed(int min, int max)
{
    fancontrollerValues.minspeed = min;
    fancontrollerValues.maxspeed = max;
    MyPreferences_setBytes("conv", &fancontrollerValues, sizeof(FanControllerValues));
}

Voltage *FanController_getFan0()
{
    return &fan0Voltage;
}

Voltage *FanController_getFan1()
{
    return &fan1Voltage;
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
            u_int16_t s = getVoltageFromPercent(fan0Voltage.max, fan0Voltage.min, val);
            volt = s;
            fan0Voltage.voltage = volt;
        }
        else if (id == 1)
        {
            u_int16_t s = getVoltageFromPercent(fan1Voltage.max, fan1Voltage.min, val);
            volt = s;
            fan1Voltage.voltage = volt;
        }
    }
    else
        volt = 0;
    log_i("set id:%i voltage to %i", id, volt);
    dac.setDACOutVoltage(volt, id);
}

void FanController_setup()
{
    Mypreferences_getBytes("fan0", &fan0Voltage, sizeof(Voltage));
    Mypreferences_getBytes("fan1", &fan1Voltage, sizeof(Voltage));
    Mypreferences_getBytes("conv", &fancontrollerValues, sizeof(FanControllerValues));
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
