# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## Project Structure

- **ESP32 firmware** (PlatformIO/Arduino): root-level `src/`, `include/`, `test/`
- **Angular 21 web UI**: `growui/src/app/` — built to `data/angular-www/` via PlatformIO post-build hook (`ngbuild.py`)

## Build & Test Commands

### ESP32 Firmware (PlatformIO)
```bash
pio run              # build firmware
pio run -t upload    # upload to device
pio monitor          # serial monitor @ 115200
```
- **Compile-time sensor flags** (in `platformio.ini` build_flags): `-DSENSOR_BME280=1`, `-DSENSOR_ENS160AHT21=1`, `-DUSE_SDCARD`, `-DMYSSID="..."`, `-DGOVEE_BTH5179=1`. These #ifdef-gate entire modules in `src/main.cpp` and source files.
- **Partition scheme**: Uses `custom_partitions.csv` (not standard ESP32 partitions).
- **Monitor filter**: `esp32_exception_decoder` is configured — stack traces will be demangled automatically.

### Angular UI (`growui/`)
```bash
cd growui && npm install
npm run start        # dev server (proxies API to device per src/proxy.conf.json)
npm run build        # production build → outputs to ../data/angular-www/
```
- **No test files exist** in `test/` directory — it is empty. Angular unit tests can be run with `npm run watch` (triggers `ng build --watch`).

### Full Firmware Build (includes UI)
PlatformIO's post-build hook (`ngbuild.py`) automatically runs `npm run build --production` inside `growui/`, then cleans up license/route files from the output. Running `pio run` alone builds both firmware and embeds the Angular assets.

## Code Style & Conventions (C++ / Arduino)

- **Naming**: snake_case for functions/variables (`sendSocketMsg()`, `g_timeZoneOffset`), PascalCase for classes (`MyWebServer`, `FanController`).
- **Includes**: angle brackets `< >` for system/framework, quotes `" "` for local headers. Conditional includes guarded by `#ifdef`.
- **Global state**: Static globals prefixed with `g_` (e.g., `g_timeZoneOffset`). Module-level globals use file-scope `static` or unnamed namespace where appropriate.
- **Logging**: Use ESP32 log macros (`log_i`, `log_e`, etc.) from esp_log.h — not `Serial.printf` for general logging (though Serial.printf is used in WebSocket callbacks).
- **I²C addresses**: Hardcoded as compile-time defines in `platformio.ini`: `BME_I2C_ADD=119`, `i2c_pwn_addr=95`, `i2c_light_addr=94`. These are decimal values (not shifted).

## Code Style & Conventions (Angular / TypeScript)

- **Strict mode enabled**: `"strict": true` in `tsconfig.json`; `noImplicitOverride`, `strictTemplates` also on.
- **Component pattern**: Standalone Angular 21 components with `@component()` decorators, signal-based inputs/outputs where applicable.
- **SCSS styles**: Per-component scoped styles (`.scss` files alongside `.ts`).
- **Proxy config**: API calls to device are proxied via `src/proxy.conf.json` — paths `/settings`, `/flashspiffs`, `/2026`, `/ws` (WebSocket).

## Key Architecture Notes

- **WiFiManager** (`src/WiFiManager.cpp`) handles AP/client mode with fallback credentials compiled into `platformio.ini`.
- **NVS storage**: Preferences stored in ESP32 NVS under namespaces like `"Voltage"`, `NVS_WIFI_NS`. Use `MyPreferences_*()` helpers — do not access Preferences directly.
- **WebSocket real-time**: AsyncWebSocket on `/ws` pushes sensor/device state to UI via JSON (`socketmsg`). See [`sendSocketMsg()`](src/main.cpp:63).
- **SPIFFS vs partition**: Firmware uses custom partition layout (not SPIFFS by default). SD card logging is optional (`#ifdef USE_SDCARD`).
