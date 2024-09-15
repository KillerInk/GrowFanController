#include "LightController.h"
#include "MyPreferences.h"

#include "DFRobot_GP8403.h"
#include "config.h"
#include "time.h"

DFRobot_GP8403 ldac(&Wire, i2c_light_addr);

LightControllerValues lvalues;

void control_light()
{
    tm time;
    getLocalTime(&time);
    //log_i("time %s",asctime(&time));
    //log_i("time %i %i %i",time.tm_year+1900, time.tm_mon+1, time.tm_mday);
    //when light is off and its time to turn it on
    if (timeEquals(time, lvalues.turnOnTime) && lvalues.current_state == off)
    {
        log_i("turn on");
        //switch to sunrise if enabled
        if (lvalues.enableSunrise && timeEqualsOrSmaler(time, lvalues.sunriseEnd))
        {
            lvalues.current_state = sunrise;
            log_i("switch to sunrise");
        }
        else // turn lamp on
        {
            lvalues.current_state = on;
            lvalues.currentLightP = lvalues.maxLightP;
            lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, lvalues.maxLightP);
        }
    }
    //check if its time to turn the light off
    else if (timeEqualsOrGreater(time, lvalues.turnOffTime) && lvalues.current_state != off)
    {
        lvalues.current_state = off;
        log_i("turn off");
        lvalues.voltage.voltage = 0;
        lvalues.currentLightP = 0;
    }
    //do sunrise stuff
    else if (lvalues.current_state == sunrise && lvalues.enableSunrise)
    {
        int timedif = ((getTimeDiff(time, lvalues.sunriseEnd) * 60) + time.tm_sec) * -1;
        int timediftotal = (getTimeDiff(lvalues.turnOnTime, lvalues.sunriseEnd) * 60) * -1;
        float p = 100 - (((float)timedif / (float)timediftotal) * 100);
        if(p > lvalues.maxLightP)
            p = lvalues.maxLightP;
        lvalues.currentLightP = p;
        lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, p);
        log_i("sunrise timedif: %i timediftotal: %i p:%f volt:%i maxv:%i minv%i", timedif, timediftotal, p, lvalues.voltage.voltage, lvalues.voltage.max, lvalues.voltage.min);

        if (timeEquals(time, lvalues.sunriseEnd))
        {
            lvalues.current_state = on;
            log_i("switch to on");
        }
    }
    //handel different stuff while lamp is on or esp rebooted and we have to continue 
    else if (lvalues.current_state == on)
    {
        if (lvalues.enableSunset && timeEqualsOrGreater(time, lvalues.sunsetStart))
        {
            lvalues.current_state = sunset;
            log_i("switch to sunset");
        }
        else if (lvalues.enableSunrise && timeEqualsOrSmaler(time, lvalues.sunriseEnd))
        {
            lvalues.current_state = sunrise;
            log_i("switch to sunrise");
        }
        else
        {
            lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, lvalues.maxLightP);
            lvalues.currentLightP =lvalues.maxLightP;
        }
        // else do nothing
    }
    //do sunset stuff
    else if (lvalues.current_state == sunset && lvalues.enableSunset)
    {
        int timedif = ((getTimeDiff(time, lvalues.sunsetStart) * 60) + time.tm_sec);
        int timediftotal = getTimeDiff(lvalues.turnOffTime, lvalues.sunsetStart) * 60;
        float p = 100 - (((float)timedif / (float)timediftotal) * 100);
        lvalues.currentLightP = p;
        lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, p);
        log_i("sunset timedif: %i timediftotal: %i p:%f volt:%i maxv:%i minv%i", timedif, timediftotal, p, lvalues.voltage.voltage, lvalues.voltage.max, lvalues.voltage.min);
    }
    else //handel fallback methods in case esp got rebooted and turn off/on event never happend but we are in the middle off something,
         // like sunrise/sunset or just to turn lamp on/off because it is in the time range
    {
        log_i("No change currenstate %i", lvalues.current_state);
        if (lvalues.current_state == off && timeEqualsOrGreater(time, lvalues.turnOnTime) && timeEqualsOrSmaler(time, lvalues.turnOffTime))
        {
            lvalues.current_state = on;
            log_i("switch to on");
        }
        else
            lvalues.current_state = off;
    }
    ldac.setDACOutVoltage(lvalues.voltage.voltage, 0);
}

void LightController_setup()
{
    Mypreferences_getBytes("light", &lvalues, sizeof(LightControllerValues));
    lvalues.current_state = off;
    ldac.setDACOutRange(ldac.eOutputRange10V);
}

void LightController_loop()
{
    if (lvalues.automode)
        control_light();
}

void LightController_setVoltageLimits(int min, int max)
{
    lvalues.voltage.min = min;
    lvalues.voltage.max = max;
    log_i("set voltage limits max %i min %i", max, min);
    MyPreferences_setBytes("light", &lvalues, sizeof(LightControllerValues));
}
void LightController_setPercentLimits(int min, int max)
{
    lvalues.minLightP = min;
    lvalues.maxLightP = max;
    log_i("set voltage limits max %i min %i", max, min);
    MyPreferences_setBytes("light", &lvalues, sizeof(LightControllerValues));
}

void LightController_setLight(int mv)
{
    if(lvalues.automode)
        return;
    lvalues.currentLightP = mv;
    lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, mv);
    log_i("set voltage %i volt %i", mv, lvalues.voltage.voltage);
    ldac.setDACOutVoltage(lvalues.voltage.voltage, 0);
}

void LightController_setAutoMode(bool active)
{
    lvalues.automode = active;
    if (!active)
        lvalues.current_state = off;
    log_i("set automode %i", active);
    MyPreferences_setBytes("light", &lvalues, sizeof(LightControllerValues));
}

LightControllerValues *LightController_getValues()
{
    return &lvalues;
}

void LightController_setTimes(int onhour, int onmin, int offhour, int offmin, int risehour, int risemin, int sethour, int setmin, bool riseenable, bool setenable)
{
    lvalues.turnOnTime.hour = onhour;
    lvalues.turnOnTime.min = onmin;
    lvalues.turnOffTime.hour = offhour;
    lvalues.turnOffTime.min = offmin;
    lvalues.sunriseEnd.hour = risehour;
    lvalues.sunriseEnd.min = risemin;
    lvalues.sunsetStart.hour = sethour;
    lvalues.sunsetStart.min = setmin;
    lvalues.enableSunrise = riseenable;
    lvalues.enableSunset = setenable;
    MyPreferences_setBytes("light", &lvalues, sizeof(LightControllerValues));
}
