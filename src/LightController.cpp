#include "LightController.h"
#include "MyPreferences.h"

#include "DFRobot_GP8403.h"
#include "config.h"
#include "time.h"

DFRobot_GP8403 ldac(&Wire, i2c_light_addr);

LightControllerValues lvalues;

void process_cloud_sim(tm time)
{
    // if cycle is new or got reseted its set to 0
    if (lvalues.next_cloud_cycle_change_time.hour == 0 && lvalues.next_cloud_cycle_change_time.min == 0)
    {
        log_i("cloud max:%i min:%i cycle:%i", lvalues.max_light_cloudP, lvalues.min_light_cloudP, lvalues.cloud_cycle_duration_min);
        // init cloud cycle based on currentlightP
        lvalues.next_cloud_cycle_change_time.hour = time.tm_hour;
        lvalues.next_cloud_cycle_change_time.min = time.tm_min;
        // 10 = 85 -75
        int rangemaxmin = lvalues.max_light_cloudP - lvalues.min_light_cloudP;
        // 5 = 85 -80
        int rangeleft = lvalues.max_light_cloudP - lvalues.currentLightP;
        // 7,5 = 15 / (10/5)
        double timeleft = (double)lvalues.cloud_cycle_duration_min / (double)((double)rangemaxmin / (double)rangeleft);
        log_i("timeleft:%f, range:%i rangeleft:%i", timeleft, rangemaxmin, rangeleft);
        addMinutes(&lvalues.next_cloud_cycle_change_time, timeleft);
    }
    else
    {
        int timedif = ((getTimeDiff(time, lvalues.next_cloud_cycle_change_time) * 60) + time.tm_sec); // sec
        if (timedif >= 0)
        {
            lvalues.cloud_rising = !lvalues.cloud_rising;
            addMinutes(&lvalues.next_cloud_cycle_change_time, lvalues.cloud_cycle_duration_min);
            log_i("changecycle");
        }
        else
        {
            MyTime startTime;
            startTime = lvalues.next_cloud_cycle_change_time;
            addMinutes(&startTime, -lvalues.cloud_cycle_duration_min);
            int timediftotal = getTimeDiff(startTime, lvalues.next_cloud_cycle_change_time) * 60;
            double p = 100 - (((double)timedif / (double)timediftotal) * 100);
            int rangemaxmin = lvalues.max_light_cloudP - lvalues.min_light_cloudP;
            double finalP = (double)rangemaxmin * (p / 100.);
            if (!lvalues.cloud_rising)
            {
                lvalues.currentLightP = lvalues.max_light_cloudP - finalP;
                lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, lvalues.max_light_cloudP - finalP);
            }
            else
            {
                lvalues.currentLightP = lvalues.min_light_cloudP + finalP;
                lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, lvalues.min_light_cloudP + finalP);
            }

            // log_i("p:%f final:%f percent:%f rising:%i", p, finalP, (lvalues.min_light_cloudP + finalP), lvalues.cloud_rising);
        }
        log_i("cloud sim timedif: %i cloudtime: %i:%i time:%i:%i volt:%i maxv:%i minv%i", timedif, lvalues.next_cloud_cycle_change_time.hour, lvalues.next_cloud_cycle_change_time.min, time.tm_hour, time.tm_min, lvalues.voltage.voltage, lvalues.voltage.max, lvalues.voltage.min);
    }
}

void control_light()
{
    tm time;
    getLocalTime(&time);
    switch (lvalues.current_state)
    {
    case off:
        // when light is off and its time to turn it on
        if (timeEqualsOrGreater(time, lvalues.turnOnTime))
        {
            log_i("turn on");
            // switch to sunrise if enabled
            if (lvalues.enableSunrise && timeEqualsOrSmaler(time, lvalues.sunriseEnd))
            {
                lvalues.current_state = sunrise;
                log_i("switch to sunrise");
            }
            else if (timeEqualsOrGreater(time, lvalues.turnOnTime) && timeEqualsOrSmaler(time, lvalues.turnOffTime)) // turn lamp on
            {
                lvalues.current_state = on;
                lvalues.currentLightP = lvalues.maxLightP;
                lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, lvalues.maxLightP);
            }
        }
        break;
    case on:
        // check if its time to turn the light off
        if (timeEqualsOrGreater(time, lvalues.turnOffTime) && lvalues.current_state != off)
        {
            lvalues.current_state = off;
            log_i("turn off");
            lvalues.voltage.voltage = 0;
            lvalues.currentLightP = 0;
        }
        else if (lvalues.enableSunset && timeEqualsOrGreater(time, lvalues.sunsetStart))
        {
            lvalues.current_state = sunset;
            log_i("switch to sunset");
        }
        else if (lvalues.enableSunrise && timeEqualsOrSmaler(time, lvalues.sunriseEnd))
        {
            lvalues.current_state = sunrise;
            log_i("switch to sunrise");
        }
        else if (lvalues.current_state == on && lvalues.voltage.voltage == 0)
        {
            lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, lvalues.maxLightP);
            lvalues.currentLightP = lvalues.maxLightP;
            lvalues.current_state = on;
        }
        else if (lvalues.cloudsim)
        {
            process_cloud_sim(time);
        }
        else if (timeEqualsOrGreater(time, lvalues.turnOnTime) && timeEqualsOrSmaler(time, lvalues.turnOffTime) && lvalues.current_state == off)
        {
            lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, lvalues.maxLightP);
            lvalues.currentLightP = lvalues.maxLightP;
            lvalues.current_state = on;
        }
        break;
    case sunrise:
        if (lvalues.enableSunrise)
        {
            int timedif = ((getTimeDiff(time, lvalues.sunriseEnd) * 60) + time.tm_sec) * -1;
            int timediftotal = (getTimeDiff(lvalues.turnOnTime, lvalues.sunriseEnd) * 60) * -1;
            double p = 100 - (((double)timedif / (double)timediftotal) * 100);
            if (p > lvalues.maxLightP)
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
        break;
    case sunset:
        if (lvalues.enableSunset)
        {
            int timedif = ((getTimeDiff(time, lvalues.sunsetStart) * 60) + time.tm_sec);
            int timediftotal = getTimeDiff(lvalues.turnOffTime, lvalues.sunsetStart) * 60;
            double p = 100 - (((double)timedif / (double)timediftotal) * 100);
            if (p > lvalues.maxLightP)
                p = lvalues.maxLightP;
            lvalues.currentLightP = p;
            lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, p);
            log_i("sunset timedif: %i timediftotal: %i p:%f volt:%i maxv:%i minv%i", timedif, timediftotal, p, lvalues.voltage.voltage, lvalues.voltage.max, lvalues.voltage.min);
        }
        break;

    default:
        break;
    }
    ldac.setDACOutVoltage(lvalues.voltage.voltage, 0);
}

void LightController_setup()
{
    Mypreferences_getBytes("light", &lvalues, sizeof(LightControllerValues));
    lvalues.current_state = off;
    ldac.setDACOutRange(ldac.eOutputRange10V);
    lvalues.next_cloud_cycle_change_time.hour = 0;
    lvalues.next_cloud_cycle_change_time.min = 0;
    log_i("cloud min:%i max:%i", lvalues.min_light_cloudP, lvalues.max_light_cloudP);
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
    if (lvalues.automode)
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

void LightController_setCloudActive(bool active)
{
    lvalues.cloudsim = active;
    MyPreferences_setBytes("light", &lvalues, sizeof(LightControllerValues));
}

void LightController_setCloudValues(int min, int max, int cycleduration)
{
    lvalues.cloud_cycle_duration_min = cycleduration;
    lvalues.min_light_cloudP = min;
    lvalues.max_light_cloudP = max;
    log_i("set cloud values cycle:%i min:%i max%i", cycleduration, min, max);
    MyPreferences_setBytes("light", &lvalues, sizeof(LightControllerValues));
    // reset timer
    lvalues.next_cloud_cycle_change_time.hour = 0;
    lvalues.next_cloud_cycle_change_time.min = 0;
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
