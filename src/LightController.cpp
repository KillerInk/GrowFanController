#include "LightController.h"
#include "MyPreferences.h"
#include "Voltage.h"
#include "DFRobot_GP8403.h"
#include "config.h"
#include "time.h"
#include "MyTime.h"

Voltage voltage;
DFRobot_GP8403 ldac(&Wire, i2c_light_addr);

MyTime turnOnTime;
MyTime turnOffTime;
MyTime sunriseEnd;
MyTime sunsetStart;
int minLightP;
int maxLightP;
int currentLightP;

bool enableSunrise = true;
bool enableSunset = false;


enum light_state
{
    off,
    on,
    sunrise,
    sunset,
};

light_state current_state = sunrise;



void control_light()
{
    tm time;
    getLocalTime(&time);
    log_i("%i %i %i",time.tm_hour,time.tm_min,time.tm_sec);
    if(timeEquals(time, turnOnTime) && current_state == off)
    {
        if(enableSunrise)
            current_state = sunrise;
        else
            current_state = on;
    
    }
    else if(timeEquals(time,turnOffTime) && current_state != off)
    {
        current_state = off;
    }
    else if(current_state == sunrise && enableSunrise)
    { 
        int timedif = getTimeDiff(time, sunriseEnd)*-1;
        int timediftotal = getTimeDiff(turnOnTime, sunriseEnd)*-1;
        int p = timediftotal * (timedif/100);
        int volt = getVoltageFromPercent(voltage.max,voltage.min, p);
        log_i("sunrise timedif: %i timediftotal: %i p:%i volt:%i", timedif, timediftotal, p, volt);

        if(timeEquals(time,sunriseEnd))
        {
            current_state = on;
        }
    }
    else if(current_state == on)
    {
        if(enableSunset && timeEquals(time, sunsetStart))
        {
            current_state == sunset;
        }
        //else do nothing
    }
    else if(current_state == sunset && enableSunset)
    {
        int timedif = getTimeDiff(time, sunsetStart);
        int timediftotal = getTimeDiff(turnOffTime, sunsetStart);
        int p = timediftotal * (timedif/100);
        int volt = getVoltageFromPercent(voltage.max,voltage.min, p);
        log_i("sunset timedif: %i timediftotal: %i p:%i volt:%i", timedif, timediftotal, p, volt);
    }
}

void LightController_setup()
{
    Mypreferences_getBytes("light",&voltage,sizeof(Voltage));
    Mypreferences_getBytes("turnon",&turnOnTime,sizeof(MyTime));
    Mypreferences_getBytes("turnoff",&turnOffTime,sizeof(MyTime));
    Mypreferences_getBytes("sunrise",&sunriseEnd,sizeof(MyTime));
    Mypreferences_getBytes("sunset",&sunsetStart,sizeof(MyTime));
}

void LightController_loop()
{
    control_light();
}

void LightController_setVoltageLimits(int min, int max)
{
    voltage.min = min;
    voltage.max = max;
    MyPreferences_setBytes("light", &voltage,sizeof(Voltage));
}

void LightController_setLight(int mv)
{
    voltage.voltage = getVoltageFromPercent(voltage.max, voltage.min, mv);
    ldac.setDACOutVoltage(voltage.voltage,0);
}

void LightController_setTimes(int onhour, int onmin,int offhour,int offmin,int risehour, int risemin, int sethour, int setmin)
{
    turnOnTime.hour = onhour;
    turnOnTime.min = onmin;
    turnOffTime.hour = offhour;
    turnOffTime.min = offmin;
    sunriseEnd.hour = risehour;
    sunriseEnd.min = risemin;
    sunsetStart.hour = sethour;
    sunsetStart.min = setmin;
    MyPreferences_setBytes("turnon", &turnOnTime,sizeof(MyTime));
    MyPreferences_setBytes("turnoff", &turnOffTime,sizeof(MyTime));
    MyPreferences_setBytes("sunset", &sunsetStart,sizeof(MyTime));
    MyPreferences_setBytes("sunrise", &sunriseEnd,sizeof(MyTime));
}
