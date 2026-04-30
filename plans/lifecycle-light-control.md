plan_id: lifecycle-light-control
status: executing

## Goal
Add cannabis plant lifecycle-aware automatic light intensity control with PPFD (μmol/m²/s) and DLI (daily light integral, cumulative μmol/m²/day) tracking to GrowControl. Light intensity automatically ramps based on the plant's growth stage and days elapsed, with real-time PPFD and cumulative DLI metrics. Includes UI for lifecycle configuration, PPFD control, and DLI progress tracking.

## Architecture — Backward Compatible Design

**Critical:** All existing LightController logic stays intact. Lifecycle is a **layer** that modifies the light target, not a replacement for the state machine.

### Integration points in control_light():
1. **off → on transition:** When switching to `on`, if `lifecycle_enabled`, call `calculateLifecycleLightP()` to get a stage-based target % instead of using `maxLightP`
2. **on state → cloud sim:** Cloud sim still works — lifecycle target acts as the *base* that cloud sim modulates around
3. **on → sunset:** Sunset ramp starts from the lifecycle-calculated target (not maxLightP)
4. **manual setLight():** Manual overrides are always respected (lifecycle only applies in automode)

### Data flow:
```
lifecycle_enabled=true → calculateLifecycleLightP() → returns target %
                                     │
                                     ▼
                    ┌─────── on ────────┐  (base for cloud sim, sunset ramp)
                    │                   │
                    ▼                   ▼
               cloud sim modulates   sunset ramp from target
               around target %       (not from maxLightP)
```

### Key principle:
- `lifecycle_enabled=false`: **identical behavior** to current code
- `lifecycle_enabled=true`: lifecycle target replaces `maxLightP` as the on-state base; all state machine transitions, sunrise/sunset/cloud sim remain unchanged

## Success Criteria
1. ✅ LifecycleStage enum + lifecycle config struct with PPFD/DLI fields (targetPPFD, dailyDLI, accumulatedDLI, umolPerWatt, panelMaxPPFD)
2. [x] Auto light ramp: light intensity calculated from days elapsed in current stage
3. [x] PPFD calculation: target PPFD derived from light intensity % × panelMaxPPFD
4. [x] DLI calculation: accumulated daily DLI tracking with stage-specific targets
5. [x] `LightController_setLifecycle()` / `LightController_getLifecycle()` APIs
6. [x] New `/cmd` parameters: `var=lifecycle` for get/set lifecycle config
7. [x] UI card in lights component for lifecycle stage + day inputs + PPFD slider + DLI progress bar
8. [x] Dashboard service + types updated for lifecycle/PPFD/DLI state
9. [ ] Persist lifecycle config via MyPreferences
10. [ ] Build and verify — compile ESP32 firmware, check growui build

## Cannabis Stage Targets (default)

| Stage | Days Range | Light % | PPFD (μmol/m²/s) | Daily DLI (μmol/m²/day) | Photoperiod |
|-------|-----------|---------|-------------------|------------------------|-------------|
| seedling | 1-21 | 20-40% | 150-250 | 4-6 | 18/6 |
| vegetative | 22-49 | 60-80% | 400-600 | 12-17 | 18/6 |
| flower_early | 50-77 | 80-95% | 600-800 | 17-21 | 12/12 |
| flower_late | 78-105 | 95-100% | 800-1000 | 21-25 | 12/12 |
| maturation | 106-112+ | 60-80% | 200-400 | 6-10 | 18/6 |

## Steps
1. [x] Extend LightController.h — add LifecycleStage enum, LifecycleConfig struct with PPFD/DLI fields, lifecycle state fields to LightControllerValues (lifecycle_enabled, stage, stageDay, stageStartTimestamp, accumulatedDLI, panelMaxPPFD), lifecycle-aware light calc function
2. [x] Implement LightController lifecycle logic in .cpp — lifecycle state machine (days tracking, stage transitions), auto light ramp (interpolate between stage min/max based on day), PPFD/DLI calculation, lifecycle API functions (set/get). **Integration:** modify control_light() on→on transition to use lifecycle target as base when enabled; sunrise/sunset/cloud sim all use this base
3. [x] Update MyWebServer.h — add lifecycle/PPFD/DLI callback pointers to MyWebServerMethodCallbacks struct
4. [x] Update MyWebServer.cpp — add /cmd handler for var=lifecycle (get/set lifecycle config via GET params)
5. [x] Update main.cpp — wire lifecycle callbacks to web server
6. [x] Update growui types.ts — add LifecycleConfig + PPFD/DLI to DeviceState interface
7. [x] Update growui api.service.ts — add lifecycle + PPFD/DLI API methods
8. [x] Update growui dashboard.service.ts — update lifecycle/PPFD/DLI state handling
9. [x] Update growui lights.component.ts + .html — add lifecycle card UI (stage selector, day counter, auto toggle, PPFD slider, DLI progress bar)
10. [ ] Build and verify — compile ESP32 firmware, check growui build

## Risks & Mitigations
- **NVS memory:** LifecycleConfig adds ~100 bytes (with PPFD/DLI) to LightControllerValues blob — need to verify flash fits in custom_partitions.csv
- **Backward compatibility:** Old clients won't have lifecycle fields — default to vegetative stage (lifecycle_enabled=false), lifecycle only active when explicitly enabled
- **Day tracking:** Need to handle ESP32 RTC reset/reboot — store lifecycle stage_start_timestamp in NVS, calculate days from timestamp difference on boot
- **Panel PPFD calibration:** panelMaxPPFD varies by hardware — make it configurable (default 1200 μmol/m²/s for typical GP8403 panel)

## Progress Log
- 2026-04-10: Plan created, PPFD/DLI added, backward-compatible design finalized, awaiting approval.
- 2026-04-10: Review complete. All source files read. Starting Step 1 — extending LightController.h.
- 2026-04-10: Steps 1-9 complete. All ESP32 C++ files updated (LightController.h/cpp, MyWebServer.h/cpp, main.cpp), all growui files updated (types.ts, api.service.ts, dashboard.service.ts, lights.component.ts/.html). Ready for build verification.
