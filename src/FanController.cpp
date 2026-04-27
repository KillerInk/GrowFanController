#include "FanController.h"
#include "DFRobot_GP8403.h"

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
double lastHumidity;
const int waitTime = 10000;

void FanController_processAutoControl()
{
    static bool lastInDeadzone = false;
    double atmp = getAvgTemp();
    double ahm = getAvgHumidity();

    int old_speed = fancontrollerValues.autocontrolfanspeed;

    // Are we currently in the deadzone (stable zone)?
    bool temp_in_deadband = atmp >= fancontrollerValues.targetTemperature - 1 && 
                            atmp <= fancontrollerValues.targetTemperature;
    bool hum_in_deadband = ahm >= fancontrollerValues.targetHumidity - 2 && 
                           ahm <= fancontrollerValues.targetHumidity;
    bool in_deadzone = temp_in_deadband && hum_in_deadband;

    // Don't adjust too often — allow only every 10s max (rate limit)
    if (millis() > nextTick) {
        if (!in_deadzone) {
            // Outside deadzone: move toward setpoint
            if (atmp > fancontrollerValues.targetTemperature || 
                ahm > fancontrollerValues.targetHumidity) {
                // Too warm/wet → increase speed
                fancontrollerValues.autocontrolfanspeed++;
            } else if (atmp < fancontrollerValues.targetTemperature - 1 || 
                       ahm < fancontrollerValues.targetHumidity - 2) {
                // Too dry/cool → decrease speed
                fancontrollerValues.autocontrolfanspeed--;
            }
        } else {
            // Inside deadzone: make tiny adjustments based on recent trend
            if (lastInDeadzone && lastTemp > atmp) 
                fancontrollerValues.autocontrolfanspeed--;
            else if (lastInDeadzone && lastTemp < atmp) 
                fancontrollerValues.autocontrolfanspeed++;
        }

        nextTick = millis() + waitTime;
    }

    lastInDeadzone = in_deadzone;

    // Clamp values
    if (fancontrollerValues.autocontrolfanspeed > fancontrollerValues.maxspeed)
        fancontrollerValues.autocontrolfanspeed = fancontrollerValues.maxspeed;
    if (fancontrollerValues.nightmodeActive && 
        fancontrollerValues.autocontrolfanspeed > fancontrollerValues.nightmodeMaxSpeed)
        fancontrollerValues.autocontrolfanspeed = fancontrollerValues.nightmodeMaxSpeed;
    if (fancontrollerValues.autocontrolfanspeed < fancontrollerValues.minspeed)
        fancontrollerValues.autocontrolfanspeed = fancontrollerValues.minspeed;

    // Update outputs only if speed changed
    if (fancontrollerValues.autocontrolfanspeed != old_speed) {
        int f0 = (fancontrollerValues.autocontrolfanspeed == 0) ? 
                 0 : getVoltageFromPercent(fancontrollerValues.fan0Voltage.max, fancontrollerValues.fan0Voltage.min, fancontrollerValues.autocontrolfanspeed);
        int f2speed = fancontrollerValues.autocontrolfanspeed - fancontrollerValues.filtercompensation;
        if (f2speed < 0) f2speed = 0;
        if (f2speed > 100) f2speed = 100;
        int f1 = (f2speed == 0) ? 
                 0 : getVoltageFromPercent(fancontrollerValues.fan1Voltage.max, fancontrollerValues.fan1Voltage.min, f2speed);

        fancontrollerValues.fan0Voltage.voltage = f0;
        fancontrollerValues.fan1Voltage.voltage = f1;

        dac.setDACOutVoltage((uint16_t)f0, 0);
        dac.setDACOutVoltage((uint16_t)f1, 1);
        log_i("autocontrol set speed to: %i fan0 mv:%u fan1 mv:%u", fancontrollerValues.autocontrolfanspeed, f0, f1);
    }

    // Save for next iteration
    lastTemp = atmp;
    lastHumidity = ahm;
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
    if (fancontrollerValues.autocontrol)
    {
        FanController_processAutoControl();
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

void FanController_applyspeed(int id, int val)
{
    int volt = 0;
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
