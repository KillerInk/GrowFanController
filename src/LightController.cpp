#include "LightController.h"
#include "MyPreferences.h"

#include "DFRobot_GP8403.h"

#include "time.h"

DFRobot_GP8403 ldac(&Wire, i2c_light_addr);

LightControllerValues lvalues;
MyTime cyclestartTime;

// Default lifecycle stage configs (cannabis growth stages)
const LifecycleStageConfig lifecycleStageConfigs[5] = {
    // seedling: days 1-21, light 20-40%, PPFD 150-250, DLI 4-6, photoperiod 18/6
    { .minLightP = 20, .maxLightP = 40, .targetPPFD = 200.0f, .dailyDLI = 5.0f, .photoperiodOnH = 6, .photoperiodOnM = 0, .photoperiodOffH = 0, .photoperiodOffM = 0 },
    // vegetative: days 22-49, light 60-80%, PPFD 400-600, DLI 12-17, photoperiod 18/6
    { .minLightP = 60, .maxLightP = 80, .targetPPFD = 500.0f, .dailyDLI = 15.0f, .photoperiodOnH = 6, .photoperiodOnM = 0, .photoperiodOffH = 0, .photoperiodOffM = 0 },
    // flower_early: days 50-77, light 80-95%, PPFD 600-800, DLI 17-21, photoperiod 12/12
    { .minLightP = 80, .maxLightP = 95, .targetPPFD = 700.0f, .dailyDLI = 19.0f, .photoperiodOnH = 0, .photoperiodOnM = 0, .photoperiodOffH = 12, .photoperiodOffM = 0 },
    // flower_late: days 78-105, light 95-100%, PPFD 800-1000, DLI 21-25, photoperiod 12/12
    { .minLightP = 95, .maxLightP = 100, .targetPPFD = 900.0f, .dailyDLI = 23.0f, .photoperiodOnH = 0, .photoperiodOnM = 0, .photoperiodOffH = 12, .photoperiodOffM = 0 },
    // maturation: days 106+, light 60-80%, PPFD 200-400, DLI 6-10, photoperiod 18/6
    { .minLightP = 60, .maxLightP = 80, .targetPPFD = 300.0f, .dailyDLI = 8.0f, .photoperiodOnH = 6, .photoperiodOnM = 0, .photoperiodOffH = 0, .photoperiodOffM = 0 },
};

// Number of days per stage
static const int stageDayRanges[5] = { 21, 28, 28, 28, 7 };

// Get the number of days in a given stage
static int getStageDayRange(lifecycle_stage stage) {
    if (stage >= 0 && stage < 5) {
        return stageDayRanges[stage];
    }
    return 14; // fallback
}

// Interpolate light % between min and max based on day progress within stage
// Returns light intensity % (0-100)
static int interpolateLightP(int minP, int maxP, int currentDay, int totalDays) {
    if (totalDays <= 1) return maxP;
    float progress = (float)(currentDay - 1) / (float)(totalDays - 1);
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;
    return (int)(minP + progress * ((float)(maxP - minP)));
}

// Calculate lifecycle-aware light target percentage
// config: lifecycle configuration
// hoursOn: hours since stage start (used for DLI accumulation)
int calculateLifecycleLightP(const LifecycleConfig *config, float hoursOn) {
    if (!config->enabled) {
        return 0; // lifecycle disabled, no target
    }

    const LifecycleStageConfig *stageConfig = &lifecycleStageConfigs[config->stage];
    int dayRange = getStageDayRange(config->stage);
    int lightP = interpolateLightP(stageConfig->minLightP, stageConfig->maxLightP, config->stageDay, dayRange);

    // Clamp to percent limits
    if (lightP < 0) lightP = 0;
    if (lightP > 100) lightP = 100;

    // Store the computed target
    ((LifecycleConfig *)config)->currentLightTargetP = lightP;

    return lightP;
}

// Internal lifecycle state update (called from control_light)
// Optimized: caches last update day to skip redundant computation
static void updateLifecycleStateInternal() {
    LifecycleConfig *lc = &lvalues.lifecycle;

    if (!lc->enabled) {
        return;
    }

    time_t now = time(nullptr);
    if (now <= 0) {
        return; // RTC not synced yet
    }

    // Calculate current calendar day number from epoch
    int currentDayNum = (int)(now / 86400);

    // Skip if we already processed this day (day-change caching)
    // Only recompute when a new day starts
    if (currentDayNum == lc->lastDLIDay && lc->lastDLIDay != 0) {
        // Still update stageDay for light target calculation, but skip DLI
        time_t elapsed = now - lc->stageStartTimestamp;
        int daysElapsed = (int)(elapsed / 86400);
        int dayInStage = daysElapsed + 1; // 1-based

        // Check stage progression (still needed - may have changed)
        int dayRange = getStageDayRange(lc->stage);
        if (dayInStage > dayRange && lc->stage < stage_maturation) {
            lc->stage = (lifecycle_stage)(lc->stage + 1);
            lc->stageDay = 1;
            lc->stageStartTimestamp = now;
            log_i("Lifecycle: advanced to stage %d", lc->stage);
            dayInStage = 1;
        }
        lc->stageDay = dayInStage;
        return; // No DLI work needed this tick
    }

    // Calculate days elapsed in current stage
    time_t elapsed = now - lc->stageStartTimestamp;
    int daysElapsed = (int)(elapsed / 86400);
    int dayInStage = daysElapsed + 1; // 1-based

    // Check if we need to advance to next stage
    int dayRange = getStageDayRange(lc->stage);
    if (dayInStage > dayRange && lc->stage < stage_maturation) {
        lc->stage = (lifecycle_stage)(lc->stage + 1);
        lc->stageDay = 1;
        lc->stageStartTimestamp = now;
        log_i("Lifecycle: advanced to stage %d", lc->stage);
        dayInStage = 1;
    }

    lc->stageDay = dayInStage;

    // New day detected - accumulate DLI
    // Accumulate DLI: estimate daily DLI from current light % and panel specs
    // DLI = PPFD × photoperiod_hours × 3600 / 1e6
    // PPFD = lightP% × panelMaxPPFD / 100
    const LifecycleStageConfig *stageConfig = &lifecycleStageConfigs[lc->stage];
    float ppfd = (float)lc->currentLightTargetP * lc->panelMaxPPFD / 100.0f;

    // Determine photoperiod hours from current stage config
    int photoperiodHours = 0;
    if (stageConfig->photoperiodOnH >= 0 && stageConfig->photoperiodOffH > 0) {
        // 12/12 photoperiod (on at 0:00, off at 12:00 → 12 hours)
        photoperiodHours = stageConfig->photoperiodOffH - stageConfig->photoperiodOnH;
        if (photoperiodHours <= 0) photoperiodHours = 12;
    } else if (stageConfig->photoperiodOnH > 0 && stageConfig->photoperiodOffH == 0) {
        // 18/6 photoperiod (on at 6:00, off at 0:00 → 18 hours)
        photoperiodHours = 24 - stageConfig->photoperiodOnH;
    } else {
        photoperiodHours = 18; // default
    }

    // DLI added per day (in μmol/m²/day)
    float dliIncrement = ppfd * (float)photoperiodHours * 3600.0f / 1000000.0f;
    lc->accumulatedDLI += dliIncrement;
    lc->lastDLIDay = currentDayNum;

    log_i("Lifecycle: stage=%d day=%d lightP=%d PPFD=%.0f DLI_day=%.1f totalDLI=%.1f",
          lc->stage, lc->stageDay, lc->currentLightTargetP, ppfd, dliIncrement, lc->accumulatedDLI);
}

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
        cyclestartTime = lvalues.next_cloud_cycle_change_time;
        addMinutes(&cyclestartTime, -(lvalues.cloud_cycle_duration_min - timeleft));
        addMinutes(&lvalues.next_cloud_cycle_change_time, timeleft);
    }
    else
    {
        int timedif = ((getTimeDiff(time, lvalues.next_cloud_cycle_change_time) * 60) + time.tm_sec); // sec
        if (timedif >= 0)
        {
            lvalues.cloud_rising = !lvalues.cloud_rising;
            cyclestartTime = lvalues.next_cloud_cycle_change_time;
            addMinutes(&lvalues.next_cloud_cycle_change_time, lvalues.cloud_cycle_duration_min);
            log_i("changecycle rising:%i time cycle end:%i:%i", lvalues.cloud_rising, lvalues.next_cloud_cycle_change_time.hour, lvalues.next_cloud_cycle_change_time.min);
        }
        else
        {
            int timediftotal = getTimeDiff(cyclestartTime, lvalues.next_cloud_cycle_change_time) * 60;
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
            if (lvalues.currentLightP < lvalues.min_light_cloudP)
            {
                lvalues.currentLightP = lvalues.min_light_cloudP;
                lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, lvalues.min_light_cloudP);
            }
            if (lvalues.currentLightP > lvalues.max_light_cloudP)
            {
                lvalues.currentLightP = lvalues.max_light_cloudP;
                lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, lvalues.max_light_cloudP);
            }
        }
    }
}

void control_light()
{
    tm time;
    getLocalTime(&time);

    // Update lifecycle state if enabled
    if (lvalues.lifecycle.enabled) {
        updateLifecycleStateInternal();
    }

    // Determine the base light target % for this state
    int baseLightP = lvalues.maxLightP; // default: use maxLightP (backward compatible)
    bool useLifecycle = false;

    // Check if lifecycle should provide the base target
    if (lvalues.lifecycle.enabled && lvalues.automode && lvalues.current_state == on) {
        useLifecycle = true;
    }

    switch (lvalues.current_state)
    {
    case off:
        if (timeEqualsOrGreater(time, lvalues.turnOnTime))
        {
            // Recalculate useLifecycle now that we're transitioning to on state
            bool lifecycleActive = lvalues.lifecycle.enabled && lvalues.automode;
            
            if (lvalues.enableSunrise && timeEqualsOrSmaller(time, lvalues.sunriseEnd) && timeEqualsOrGreater(time, lvalues.turnOnTime))
            {
                lvalues.current_state = sunrise;
                log_i("switch to sunrise");
            }
            else if (timeEqualsOrGreater(time, lvalues.turnOnTime) && timeGreater(lvalues.turnOnTime, lvalues.turnOffTime) ? timeGreater(time, lvalues.turnOffTime) : timeSmaller(time, lvalues.turnOffTime))
            {
                log_i("turn on");
                lvalues.current_state = on;

                if (lifecycleActive) {
                    baseLightP = calculateLifecycleLightP(&lvalues.lifecycle, 0);
                    lvalues.currentLightP = baseLightP;
                    lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, baseLightP);
                    log_i("Lifecycle light: stage=%d day=%d target=%d%%", lvalues.lifecycle.stage, lvalues.lifecycle.stageDay, baseLightP);
                } else {
                    lvalues.currentLightP = lvalues.maxLightP;
                    lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, lvalues.maxLightP);
                }

                if (lvalues.cloudsim)
                {
                    lvalues.next_cloud_cycle_change_time.hour = 0;
                    lvalues.next_cloud_cycle_change_time.min = 0;
                }
            }
        }
        break;
    case on:
        if (timeEqualsOrGreater(time, lvalues.turnOffTime) && timeGreater(lvalues.turnOffTime, lvalues.turnOnTime) ? timeGreater(time, lvalues.turnOnTime) : timeSmaller(time, lvalues.turnOnTime))
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
        else if (lvalues.voltage.voltage == 0)
        {
            if (useLifecycle) {
                baseLightP = lvalues.lifecycle.currentLightTargetP;
                if (baseLightP <= 0) baseLightP = lvalues.maxLightP;
            }
            lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, baseLightP);
            lvalues.currentLightP = baseLightP;
            lvalues.current_state = on;
        }
        else if (lvalues.cloudsim)
        {
            process_cloud_sim(time);
        }
        break;
    case sunrise:
        if (lvalues.enableSunrise)
        {
            int timedif = ((getTimeDiff(time, lvalues.sunriseEnd) * 60) + time.tm_sec) * -1;
            int timediftotal = (getTimeDiff(lvalues.turnOnTime, lvalues.sunriseEnd) * 60) * -1;
            double p = 100 - (((double)timedif / (double)timediftotal) * 100);
            int rampMax = useLifecycle ? lvalues.lifecycle.currentLightTargetP : lvalues.maxLightP;
            if (rampMax <= 0) rampMax = lvalues.maxLightP;
            if (p > rampMax) p = rampMax;
            lvalues.currentLightP = (int)p;
            lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, (int)p);
            log_i("sunrise timedif: %i timediftotal: %i p:%f volt:%i", timedif, timediftotal, p, lvalues.voltage.voltage);

            if (timeEquals(time, lvalues.sunriseEnd) || timedif < 0)
            {
                lvalues.current_state = on;
                log_i("switch to on");
                if (lvalues.cloudsim)
                {
                    lvalues.next_cloud_cycle_change_time.hour = 0;
                    lvalues.next_cloud_cycle_change_time.min = 0;
                }
            }
        }
        break;
    case sunset:
        if (lvalues.enableSunset)
        {
            int timedif = ((getTimeDiff(time, lvalues.sunsetStart) * 60) + time.tm_sec);
            int rampMax = useLifecycle ? lvalues.lifecycle.currentLightTargetP : lvalues.maxLightP;
            if (rampMax <= 0) rampMax = lvalues.maxLightP;
            int timediftotal = getTimeDiff(lvalues.turnOffTime, lvalues.sunsetStart) * 60;
            double p = 100 - (((double)timedif / (double)timediftotal) * 100);
            if (p > rampMax) p = rampMax;
            lvalues.currentLightP = (int)p;
            lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, (int)p);
            log_i("sunset timedif: %i timediftotal: %i p:%f volt:%i", timedif, timediftotal, p, lvalues.voltage.voltage);
            if (timeEquals(time, lvalues.turnOffTime))
            {
                lvalues.current_state = off;
                lvalues.voltage.voltage = 0;
                lvalues.currentLightP = 0;
            }
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

    // Initialize lifecycle defaults if not set
    if (lvalues.lifecycle.panelMaxPPFD == 0) {
        lvalues.lifecycle.panelMaxPPFD = 1200.0f;
    }
    if (lvalues.lifecycle.umolPerWatt == 0) {
        lvalues.lifecycle.umolPerWatt = 2.0f;
    }
    if (lvalues.lifecycle.enabled && lvalues.lifecycle.stageStartTimestamp == 0) {
        lvalues.lifecycle.stageStartTimestamp = time(nullptr);
        if (lvalues.lifecycle.stageStartTimestamp <= 0) {
            lvalues.lifecycle.stageStartTimestamp = 1700000000;
        }
    }
    
    // Initialize lastDLIDay to current day if not yet set (avoids backlog on first boot)
    if (lvalues.lifecycle.lastDLIDay == 0) {
        time_t now = time(nullptr);
        if (now > 0) {
            lvalues.lifecycle.lastDLIDay = (int)(now / 86400);
        }
    }

    lvalues.current_state = off;
    lvalues.voltage.voltage = 0;
    lvalues.currentLightP = 0;
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
    log_i("set percent limits max %i min %i", max, min);
    MyPreferences_setBytes("light", &lvalues, sizeof(LightControllerValues));
}

void LightController_setLight(int mv)
{
    // Disable automode when manual light control is requested
    if (lvalues.automode) {
        lvalues.automode = false;
        lvalues.current_state = off;
        log_i("Automode disabled by manual light control");
    }
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
    lvalues.next_cloud_cycle_change_time.hour = 0;
    lvalues.next_cloud_cycle_change_time.min = 0;
    MyPreferences_setBytes("light", &lvalues, sizeof(LightControllerValues));
}

void LightController_setCloudValues(int min, int max, int cycleduration)
{
    lvalues.cloud_cycle_duration_min = cycleduration;
    lvalues.min_light_cloudP = min;
    lvalues.max_light_cloudP = max;
    log_i("set cloud values cycle:%i min:%i max%i", cycleduration, min, max);
    MyPreferences_setBytes("light", &lvalues, sizeof(LightControllerValues));
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

// ===== Lifecycle API Functions =====

void LightController_setLifecycleEnabled(bool enabled)
{
    lvalues.lifecycle.enabled = enabled;
    if (enabled && lvalues.lifecycle.stageStartTimestamp == 0) {
        lvalues.lifecycle.stageStartTimestamp = time(nullptr);
    }
    log_i("Lifecycle %s", enabled ? "enabled" : "disabled");
    MyPreferences_setBytes("light", &lvalues, sizeof(LightControllerValues));
}

void LightController_setLifecycleStage(lifecycle_stage stage)
{
    if (stage >= stage_seedling && stage <= stage_maturation) {
        lvalues.lifecycle.stage = stage;
        lvalues.lifecycle.stageDay = 1;
        lvalues.lifecycle.stageStartTimestamp = time(nullptr);
        lvalues.lifecycle.accumulatedDLI = 0;
        lvalues.lifecycle.lastDLIDay = 0;  // Reset so DLI starts fresh on next update
        log_i("Lifecycle: set stage to %d", stage);
        MyPreferences_setBytes("light", &lvalues, sizeof(LightControllerValues));
    }
}

void LightController_resetLifecycleStage()
{
    lvalues.lifecycle.stage = stage_vegetative;
    lvalues.lifecycle.stageDay = 1;
    lvalues.lifecycle.stageStartTimestamp = time(nullptr);
    lvalues.lifecycle.accumulatedDLI = 0;
    lvalues.lifecycle.lastDLIDay = 0;  // Reset DLI tracking so it starts fresh
    log_i("Lifecycle: reset to vegetative stage");
    MyPreferences_setBytes("light", &lvalues, sizeof(LightControllerValues));
}


LifecycleConfig *LightController_getLifecycleConfig()
{
    return &lvalues.lifecycle;
}

void LightController_updateLifecycleState()
{
    updateLifecycleStateInternal();
}
