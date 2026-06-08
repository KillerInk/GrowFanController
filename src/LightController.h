#pragma once
#include "MyTime.h"
#include "Voltage.h"
#include <time.h>

enum light_state
{
    off,
    on,
    sunrise,
    sunset,
};

// Lifecycle stages for cannabis growth
enum lifecycle_stage
{
    stage_seedling = 0,
    stage_vegetative = 1,
    stage_flower_early = 2,
    stage_flower_late = 3,
    stage_maturation = 4,
};

// Cannabis plant type (affects light schedule behavior)
enum plant_type
{
    plant_photoperiodic = 0,  // Responds to day length (traditional)
    plant_automatic = 1,      // Autoflowering (fixed daily schedule)
};

// Stage-specific light configuration
typedef struct {
    int minLightP;          // min light intensity % for this stage
    int maxLightP;          // max light intensity % for this stage
    float targetPPFD;       // target PPFD in μmol/m²/s (for reference)
    float dailyDLI;         // target daily DLI in μmol/m²/day
    int photoperiodOnH;     // light on hours for photoperiod
    int photoperiodOnM;
    int photoperiodOffH;
    int photoperiodOffM;
} LifecycleStageConfig;

// Lifecycle configuration (stored in NVS)
typedef struct {
    bool enabled;
    lifecycle_stage stage;
    plant_type plantType;       // plant type (photoperiodic or automatic)
    int stageDay;                   // day within current stage (1-based)
    time_t stageStartTimestamp;     // epoch time when stage started
    float accumulatedDLI;           // cumulative DLI across all stages (μmol/m²/day)
    float panelMaxPPFD;             // panel max PPFD in μmol/m²/s (calibrated, default 1200)
    float umolPerWatt;              // panel efficiency (μmol/J), default 2.0
    int currentLightTargetP;        // computed light target % (updated each loop)
    int32_t lastDLIDay;             // epoch day (now/86400) when DLI was last updated
    uint8_t padding[1];             // align to 4-byte boundary
} LifecycleConfig;

// Default stage configs (cannabis-specific)
extern const LifecycleStageConfig lifecycleStageConfigs[5];

// Lifecycle-aware light calculation
int calculateLifecycleLightP(LifecycleConfig *config, float hoursOn);

struct LightControllerValues
{
    Voltage voltage;
    MyTime turnOnTime;
    MyTime turnOffTime;
    MyTime sunriseEnd;
    MyTime sunsetStart;
    int minLightP = 20;
    int maxLightP = 80;
    int currentLightP;

    bool enableSunrise = false;
    bool enableSunset = false;

    light_state current_state = off;
    bool automode = false;

    bool cloudsim = false;
    int min_light_cloudP = 75;
    int max_light_cloudP = 85;
    int cloud_cycle_duration_min = 15;
    bool cloud_falling = false;
    MyTime next_cloud_cycle_change_time;
    bool cloudCycleInitialized = false;  // Track cloud cycle init state (fixes fragile zero-check)

    // Lifecycle fields (added 2026-04-10)
    LifecycleConfig lifecycle;
};

void LightController_setup();
void LightController_loop();
void LightController_setVoltageLimits(int min, int max);
void LightController_setPercentLimits(int min, int max);
void LightController_setTimes(int onhour, int onmin, int offhour, int offmin, int risehour, int risemin, int sethour, int setmin,bool riseenable, bool setenable);
void LightController_setLight(int lightPct);
void LightController_setAutoMode(bool active);
void LightController_setCloudActive(bool active);
void LightController_setCloudValues(int min, int max, int cycleduration);
// Lifecycle APIs
void LightController_setLifecycleEnabled(bool enabled);
void LightController_setLifecycleStage(lifecycle_stage stage);
void LightController_resetLifecycleStage();
void LightController_setPlantType(plant_type type);
LifecycleConfig *LightController_getLifecycleConfig();
void LightController_updateLifecycleState();

LightControllerValues * LightController_getValues();
