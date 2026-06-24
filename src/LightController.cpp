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
    { .minLightP = 20, .maxLightP = 40, .targetPPFD = 150.0f, .dailyDLI = 5.0f, .photoperiodOnH = 6, .photoperiodOnM = 0, .photoperiodOffH = 0, .photoperiodOffM = 6 },
    // vegetative: days 22-49, light 60-80%, PPFD 400-600, DLI 12-17, photoperiod 18/6
    { .minLightP = 60, .maxLightP = 80, .targetPPFD = 500.0f, .dailyDLI = 15.0f, .photoperiodOnH = 6, .photoperiodOnM = 0, .photoperiodOffH = 0, .photoperiodOffM = 6 },
    // flower_early: days 50-77, light 80-95%, PPFD 600-800, DLI 17-21, photoperiod 12/12
    { .minLightP = 80, .maxLightP = 95, .targetPPFD = 700.0f, .dailyDLI = 19.0f, .photoperiodOnH = 0, .photoperiodOnM = 0, .photoperiodOffH = 12, .photoperiodOffM = 0 },
    // flower_late: days 78-105, light 95-100%, PPFD 800-1000, DLI 21-25, photoperiod 12/12
    { .minLightP = 95, .maxLightP = 100, .targetPPFD = 900.0f, .dailyDLI = 23.0f, .photoperiodOnH = 0, .photoperiodOnM = 0, .photoperiodOffH = 12, .photoperiodOffM = 0 },
    // maturation: days 106+, light 60-80%, PPFD 200-400, DLI 6-10, photoperiod 18/6
    { .minLightP = 60, .maxLightP = 80, .targetPPFD = 300.0f, .dailyDLI = 8.0f, .photoperiodOnH = 6, .photoperiodOnM = 0, .photoperiodOffH = 0, .photoperiodOffM = 6 },
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
// config: lifecycle configuration (non-const to allow writing currentLightTargetP)
// Returns light intensity % (0-100)
int calculateLifecycleLightP(LifecycleConfig *config, float hoursOn) {
    if (!config->enabled) {
        return 0; // lifecycle disabled, no target
    }

    const LifecycleStageConfig *stageConfig = &lifecycleStageConfigs[config->stage];
    int dayRange = getStageDayRange(config->stage);
    int lightP = interpolateLightP(stageConfig->minLightP, stageConfig->maxLightP, config->stageDay, dayRange);

    // Clamp to percent limits
    if (lightP < 0) lightP = 0;
    if (lightP > 100) lightP = 100;

    // Store the computed target (no const-cast needed)    config->currentLightTargetP = lightP;

    return lightP;
}

// ===== Forward declarations =====
static void process_cloud_sim(tm time);

// ===== Helper: is 'now' within the light-on period? =====
// Handles both same-day (e.g., 06:00-22:00) and cross-midnight (e.g., 22:00-06:00 next day)
static bool isLightOnPeriod(tm now, MyTime onTime, MyTime offTime) {
    if (onTime.hour < offTime.hour) {
        // Same day: on at 06:00, off at 22:00
        return timeEqualsOrGreater(now, onTime) && timeSmaller(now, offTime);
    } else if (onTime.hour > offTime.hour) {
        // Cross-midnight: on at 22:00, off at 06:00
        return timeEqualsOrGreater(now, onTime) || timeSmaller(now, offTime);
    }
    return false; // onTime == offTime means light never turns on
}

// ===== State transition evaluation =====
// Returns the next state to transition to, or current state if no transition needed
static light_state evaluate_next_state(tm now) {
    switch (lvalues.current_state) {
    case off:
        if (isLightOnPeriod(now, lvalues.turnOnTime, lvalues.turnOffTime)) {
            if (lvalues.enableSunrise) {
                return sunrise;
            }
            return on;
        }
        return off;

    case on:
        if (!isLightOnPeriod(now, lvalues.turnOnTime, lvalues.turnOffTime)) {
            if (lvalues.enableSunset && timeEqualsOrGreater(now, lvalues.sunsetStart) && timeSmaller(now, lvalues.turnOffTime)) {
                return sunset;
            }
            return off;
        }
        return on;

    case sunrise:
        if (!lvalues.enableSunrise || timeEqualsOrGreater(now, lvalues.sunriseEnd)) {
            return on;
        }
        return sunrise;

    case sunset:
        // Only transition to sunset if we're within the valid window:
        // sunsetStart <= now < turnOffTime
        if (!lvalues.enableSunset || timeEqualsOrGreater(now, lvalues.turnOffTime) || timeSmaller(now, lvalues.sunsetStart)) {
            return off;
        }
        return sunset;

    default:
        return off;
    }
}

// ===== Compute light intensity for a given state =====
static void compute_state_output(tm now) {
    int outputVoltage;
    int lightP;

    switch (lvalues.current_state) {
    case off:
        lightP = 0;
        outputVoltage = 0;
        break;

    case on: {
        // Determine base light target
        int baseLightP = lvalues.maxLightP; // default: use maxLightP (backward compatible)
        bool useLifecycle = lvalues.lifecycle.enabled && lvalues.automode;
        
        if (useLifecycle) {
            baseLightP = lvalues.lifecycle.currentLightTargetP;
            if (baseLightP <= 0) baseLightP = lvalues.maxLightP;
        }

        lightP = baseLightP;
        outputVoltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, baseLightP);

        // Handle cloud simulation
        if (lvalues.cloudsim) {
            process_cloud_sim(now);
            lightP = lvalues.currentLightP;
            outputVoltage = lvalues.voltage.voltage;
        }
        break;
    }

    case sunrise:
        if (lvalues.enableSunrise) {
            // Calculate elapsed time since sunrise start
            // getTimeDiff(now, sunriseEnd) = now - sunriseEnd (negative while now < sunriseEnd)
            int timedifSec = (getTimeDiff(now, lvalues.sunriseEnd) * 60) + now.tm_sec;
            int timediftotalSec = (getTimeDiff(lvalues.turnOnTime, lvalues.sunriseEnd) * 60);
            
            // timediftotalSec is negative (turnOnTime < sunriseEnd)
            if (timediftotalSec >= 0) {
                lightP = lvalues.maxLightP;
                outputVoltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, lvalues.maxLightP);
            } else {
                // elapsed = (now - sunriseEnd) - (turnOnTime - sunriseEnd) = now - turnOnTime
                // This gives 0 at start, totalDuration at end
                int elapsedSec = timedifSec - timediftotalSec;
                int totalSec = -timediftotalSec;
                double p = ((double)elapsedSec / (double)totalSec) * 100.0;
                
                // Clamp percentage to 0-100 range
                if (p < 0.0) p = 0.0;
                if (p > 100.0) p = 100.0;
                
                int rampMax = lvalues.maxLightP;
                if (lvalues.lifecycle.enabled && lvalues.automode) {
                    rampMax = lvalues.lifecycle.currentLightTargetP;
                    if (rampMax <= 0) rampMax = lvalues.maxLightP;
                }
                if (p > rampMax) p = rampMax;
                if (p < 0) p = 0;
                
                lightP = (int)p;
                outputVoltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, (int)p);
                log_i("sunrise elapsed:%i total:%i p:%f volt:%i", elapsedSec, totalSec, p, outputVoltage);
            }
        } else {
            lightP = 0;
            outputVoltage = 0;
        }
        break;

    case sunset:
        if (lvalues.enableSunset) {
            // Calculate time elapsed since sunset started
            int timedifSec = (getTimeDiff(now, lvalues.sunsetStart) * 60) + now.tm_sec;
            
            // Clamp timedif to non-negative
            if (timedifSec < 0) timedifSec = 0;
            
            int timediftotalSec = getTimeDiff(lvalues.turnOffTime, lvalues.sunsetStart) * 60;
            
            // Guard: if sunset hasn't started yet or turnOffTime <= sunsetStart, skip
            if (timediftotalSec <= 0 || timedifSec == 0) {
                // Check if we're still within the valid sunset window
                if (timeEqualsOrGreater(now, lvalues.sunsetStart) && timediftotalSec > 0) {
                    timedifSec = 1; // Minimum non-zero to start ramping
                } else {
                    lightP = lvalues.maxLightP;
                    outputVoltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, lvalues.maxLightP);
                    break;
                }
            }
            
            int rampMax = lvalues.maxLightP;
            if (lvalues.lifecycle.enabled && lvalues.automode) {
                rampMax = lvalues.lifecycle.currentLightTargetP;
                if (rampMax <= 0) rampMax = lvalues.maxLightP;
            }
            
            double p = 100.0 - (((double)timedifSec / (double)timediftotalSec) * 100.0);
            
            // Clamp percentage to valid range
            if (p < 0.0) p = 0.0;
            if (p > 100.0) p = 100.0;
            if (p > rampMax) p = rampMax;
            if (p < 0) p = 0;
            
            lightP = (int)p;
            outputVoltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, (int)p);
            log_i("sunset elapsed:%i total:%i p:%f volt:%i", timedifSec, timediftotalSec, p, outputVoltage);
        } else {
            lightP = 0;
            outputVoltage = 0;
        }
        break;

    default:
        lightP = 0;
        outputVoltage = 0;
        break;
    }

    // Apply output with change detection (avoids unnecessary I2C writes)
    static int lastOutputVoltage = -1;
    static light_state lastOutputState = off;
    if (outputVoltage != lastOutputVoltage || lvalues.current_state != lastOutputState) {
        ldac.setDACOutVoltage(outputVoltage, 0);
        lastOutputVoltage = outputVoltage;
        lastOutputState = lvalues.current_state;
    }

    lvalues.currentLightP = lightP;
    lvalues.voltage.voltage = outputVoltage;
}

// ===== Internal lifecycle state update =====
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
        int currentStageDayNum = (int)(lc->stageStartTimestamp / 86400);
        int daysElapsed = currentDayNum - currentStageDayNum;
        if (daysElapsed < 0) daysElapsed = 0;
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

    // Calculate days elapsed in current stage using calendar day numbers
    int currentStageDayNum = (int)(lc->stageStartTimestamp / 86400);
    int daysElapsed = currentDayNum - currentStageDayNum;
    if (daysElapsed < 0) daysElapsed = 0;
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
    // For photoperiodic plants: use stage config's photoperiod hours
    // For automatic (autoflowering) plants: use device's configured light-on hours
    int photoperiodHours = 0;
    if (lc->plantType == plant_automatic) {
        // Automatic: calculate from device's turnOnTime/turnOffTime
        int onHour = lvalues.turnOnTime.hour;
        int onMin = lvalues.turnOnTime.min;
        int offHour = lvalues.turnOffTime.hour;
        int offMin = lvalues.turnOffTime.min;
        
        if (offHour > onHour || (offHour == onHour && offMin > onMin)) {
            // Same-day: e.g., on 06:00, off 22:00
            photoperiodHours = offHour - onHour + ((offMin - onMin) / 60);
        } else if (offHour < onHour || (offHour == onHour && offMin < onMin)) {
            // Cross-midnight: e.g., on 22:00, off 06:00 (next day)
            photoperiodHours = (24 - onHour) + offHour + ((offMin - onMin) / 60);
        } else {
            // onTime == offTime: full 24 hours
            photoperiodHours = 24;
        }
        if (photoperiodHours <= 0) photoperiodHours = 18;
    } else {
        // Photoperiodic: use stage config's photoperiod hours
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
    double timeleft;

    // Use cloudCycleInitialized flag instead of checking for zero values
    // This fixes the bug where system boot at 00:00 triggers false initialization
    if (!lvalues.cloudCycleInitialized) {
        log_i("cloud max:%i min:%i cycle:%i", lvalues.max_light_cloudP, lvalues.min_light_cloudP, lvalues.cloud_cycle_duration_min);
        
        // Init cloud cycle based on current light state
        lvalues.next_cloud_cycle_change_time.hour = time.tm_hour;
        lvalues.next_cloud_cycle_change_time.min = time.tm_min;
        
        // 10 = 85 -75
        int rangemaxmin = lvalues.max_light_cloudP - lvalues.min_light_cloudP;
        // 5 = 85 -80
        int rangeleft = lvalues.max_light_cloudP - lvalues.currentLightP;
        // Guard against division by zero when currentLightP == max_light_cloudP
        if (rangeleft == 0) {
            timeleft = (double)lvalues.cloud_cycle_duration_min;
        } else {
            timeleft = (double)lvalues.cloud_cycle_duration_min / (double)((double)rangemaxmin / (double)rangeleft);
        }
        log_i("timeleft:%f, range:%i rangeleft:%i", timeleft, rangemaxmin, rangeleft);
        cyclestartTime = lvalues.next_cloud_cycle_change_time;
        addMinutes(&cyclestartTime, -(lvalues.cloud_cycle_duration_min - timeleft));
        addMinutes(&lvalues.next_cloud_cycle_change_time, timeleft);
        
        lvalues.cloudCycleInitialized = true;
    }
    else
    {
        int timedif = ((getTimeDiff(time, lvalues.next_cloud_cycle_change_time) * 60) + time.tm_sec); // sec
        if (timedif >= 0)
        {
            lvalues.cloud_falling = !lvalues.cloud_falling;
            cyclestartTime = lvalues.next_cloud_cycle_change_time;
            addMinutes(&lvalues.next_cloud_cycle_change_time, lvalues.cloud_cycle_duration_min);
            log_i("changecycle falling:%i time cycle end:%i:%i", lvalues.cloud_falling, lvalues.next_cloud_cycle_change_time.hour, lvalues.next_cloud_cycle_change_time.min);
        }
        else
        {
            int timediftotal = getTimeDiff(cyclestartTime, lvalues.next_cloud_cycle_change_time) * 60;
            double p = 100 - (((double)timedif / (double)timediftotal) * 100);
            int rangemaxmin = lvalues.max_light_cloudP - lvalues.min_light_cloudP;
            double finalP = (double)rangemaxmin * (p / 100.);
            
            // Clamp cloud bounds to lifecycle target when lifecycle is enabled
            int cloudMin = lvalues.min_light_cloudP;
            int cloudMax = lvalues.max_light_cloudP;
            if (lvalues.lifecycle.enabled && lvalues.automode && lvalues.lifecycle.currentLightTargetP > 0) {
                int target = lvalues.lifecycle.currentLightTargetP;
                cloudMin = (target < cloudMin) ? target : cloudMin;
                cloudMax = (target > cloudMax) ? target : cloudMax;
            }
            rangemaxmin = cloudMax - cloudMin;
            
            // cloud_falling: false = rising (min→max), true = falling (max→min)
            if (!lvalues.cloud_falling)
            {
                lvalues.currentLightP = cloudMax - finalP;
                lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, cloudMax - finalP);
            }
            else
            {
                lvalues.currentLightP = cloudMin + finalP;
                lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, cloudMin + finalP);
            }
            if (lvalues.currentLightP < cloudMin)
            {
                lvalues.currentLightP = cloudMin;
                lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, cloudMin);
            }
            if (lvalues.currentLightP > cloudMax)
            {
                lvalues.currentLightP = cloudMax;
                lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, cloudMax);
            }
        }
    }
}

// ===== Main light control function (refactored) =====
// Separates transition evaluation from state output computation
void control_light()
{
    // Only get time if automode is enabled (optimization)
    if (!lvalues.automode) return;
    
    tm time;
    getLocalTime(&time);

    // Update lifecycle state if enabled
    if (lvalues.lifecycle.enabled) {
        updateLifecycleStateInternal();
    }

    // Compute currentLightTargetP here so it reflects the actual state
    if (lvalues.lifecycle.enabled && lvalues.automode) {
        calculateLifecycleLightP(&lvalues.lifecycle, 0);
    }

    // Evaluate state transition (separated from output computation)
    light_state nextState = evaluate_next_state(time);

    // Apply transition if state changed
    if (nextState != lvalues.current_state) {
        lvalues.current_state = nextState;
        
        // Log state transition
        const char *stateNames[] = {"off", "on", "sunrise", "sunset"};
        log_i("state -> %s", stateNames[nextState]);

        // Handle state entry actions
        switch (nextState) {
        case on:
            // Initialize cloud cycle when entering 'on' state
            if (lvalues.cloudsim) {
                lvalues.next_cloud_cycle_change_time.hour = 0;
                lvalues.next_cloud_cycle_change_time.min = 0;
                lvalues.cloudCycleInitialized = false;  // Reset for next entry
            }
            break;
            
        case off:
            // Reset cloud cycle tracking when turning off
            lvalues.cloudCycleInitialized = false;
            break;
            
        default:
            break;
        }
    }

    // Compute and apply state output
    compute_state_output(time);
}

void LightController_setup()
{
    // Initialize struct to defaults BEFORE NVS read to prevent legacy data corruption
    memset(&lvalues, 0, sizeof(LightControllerValues));
    lvalues.voltage.min = 0;
    lvalues.voltage.max = 1000;
    lvalues.minLightP = 20;
    lvalues.maxLightP = 80;
    lvalues.turnOnTime.hour = 6;
    lvalues.turnOnTime.min = 0;
    lvalues.turnOffTime.hour = 22;
    lvalues.turnOffTime.min = 0;
    lvalues.sunriseEnd.hour = 6;
    lvalues.sunriseEnd.min = 30;
    lvalues.sunsetStart.hour = 21;
    lvalues.sunsetStart.min = 0;
    lvalues.enableSunrise = false;
    lvalues.enableSunset = false;
    lvalues.cloudsim = false;
    lvalues.min_light_cloudP = 75;
    lvalues.max_light_cloudP = 85;
    lvalues.cloud_cycle_duration_min = 15;
    lvalues.automode = false;
    lvalues.current_state = off;
    lvalues.lifecycle.enabled = false;
    lvalues.lifecycle.plantType = plant_photoperiodic;
    
    // Now read NVS — only overwrites fields that were actually stored
    Mypreferences_getBytes("light", &lvalues, sizeof(LightControllerValues));

    ldac.setDACOutRange(ldac.eOutputRange10V);

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
    
    // Initialize lastDLIDay to current day if not yet set
    if (lvalues.lifecycle.lastDLIDay == 0) {
        time_t now = time(nullptr);
        if (now > 0) {
            lvalues.lifecycle.lastDLIDay = (int)(now / 86400);
            // Estimate partial-day DLI based on current light state
            if (lvalues.currentLightP > 0 && lvalues.automode) {
                const LifecycleStageConfig *stageConfig = &lifecycleStageConfigs[lvalues.lifecycle.stage];
                float ppfd = (float)lvalues.currentLightP * lvalues.lifecycle.panelMaxPPFD / 100.0f;
                // Estimate hours already on today (simplified: assume half-day if lights are on)
                float partialHours = 12.0f; // conservative estimate
                float partialDLI = ppfd * partialHours * 3600.0f / 1000000.0f;
                lvalues.lifecycle.accumulatedDLI += partialDLI;
                log_i("Lifecycle: boot partial DLI added %.1f", partialDLI);
            }
        }
    }
    
    // Initialize plant type to photoperiodic if not set (default)
    if (lvalues.lifecycle.plantType != plant_automatic) {
        lvalues.lifecycle.plantType = plant_photoperiodic;
    }

    lvalues.current_state = off;
    lvalues.voltage.voltage = 0;
    lvalues.currentLightP = 0;
    lvalues.next_cloud_cycle_change_time.hour = 0;
    lvalues.next_cloud_cycle_change_time.min = 0;
    lvalues.cloudCycleInitialized = false;  // Ensure fresh start
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

void LightController_setLight(int lightPct)
{
    // Disable automode when manual light control is requested
    if (lvalues.automode) {
        lvalues.automode = false;
        lvalues.current_state = off;
        log_i("Automode disabled by manual light control");
    }
    // Clamp to valid range
    if (lightPct < 0) lightPct = 0;
    if (lightPct > 100) lightPct = 100;
    lvalues.currentLightP = lightPct;
    lvalues.voltage.voltage = getVoltageFromPercent(lvalues.voltage.max, lvalues.voltage.min, lightPct);
    log_i("set light %d%% volt %i", lightPct, lvalues.voltage.voltage);
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
    lvalues.cloudCycleInitialized = false;  // Reset cycle on re-enable
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
    lvalues.cloudCycleInitialized = false;  // Reset on config change
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

void LightController_setPlantType(plant_type type)
{
    if (type == plant_photoperiodic || type == plant_automatic) {
        lvalues.lifecycle.plantType = type;
        log_i("Plant type set to %s", type == plant_photoperiodic ? "photoperiodic" : "automatic");
        MyPreferences_setBytes("light", &lvalues, sizeof(LightControllerValues));
    }
}

LifecycleConfig *LightController_getLifecycleConfig()
{
    return &lvalues.lifecycle;
}

void LightController_updateLifecycleState()
{
    updateLifecycleStateInternal();
}
