# Esp Grow Controller

![](/connect.jpg)

## Using an Esp32 to control various different devices for a GrowTent.
### Hardware used:

1. Esp32 1x
2. Gravity: 2-Channel I2C DAC Module (0-10V) 2x
3. SD CARD Reader 
4. ENS160ATH21 Sensor or BME280
5. 12v stepdown to 5v

### Pins
I2C 
    SDA 21
    SCL 22
SDCARD
    CS 5
    MOSI 23
    SCK 18
    MISO 19

### Features:
1. Can control 2 different Fans(in/out) based on Temperature and Humidity and apply a speed difference to keep a negative pressure
2. Set a nightmode to reduce the fan speed for that time range
3. Automatic turn light on/off and apply sunrise/set
4. Log humidity, temperature, co2, .... to sdcard, it create files in that structure /year/month/day/hour.csv (/2024/03/05/12.csv) and store a value every sec
5. webinterface with a chart

![](/web.jpg)

### Notes about DAC Module
Every devices that use the same Power Source as the Esp is connected to,
only need the PWM signal and no GND.
Extern powered devices like the lamp(12v) or the big fan(24v) need a connection to GND on the DAC!


## Build and Flash

### Prerequisites
- **PlatformIO Core** (or the VS Code PlatformIO extension):  
  `pip install platformio`  *(or follow instructions at https://platformio.org/install)*.
- USB cable to connect your ESP‑32.

### 1. Update Environment (optional)

If your ESP‑32 is on a different IP, edit **src/environments/environment.ts**:

```ts
export const environment = {
  production: false,
  apiBaseUrl: 'http://<ESP_IP>',   // <-- host of the web server
  apiPort: 80,                     // <-- http_port value
};
```

### 2. Build Firmware

## Build Flags & Configuration

The firmware can be tailored at compile time via the following `-D` flags in **platformio.ini**.  
Uncomment a flag by removing the leading semicolon (`;`) or pass it directly with `pio run -d`.

| Flag | Purpose |
|------|---------|
| `USE_SDCARD=1` | Enable SD‑card logging. The firmware will create `/year/month/day/hour.csv` files on the card. |
| `SENSOR_ENS160AHT21=1` | Use an ENS160 / AHT21 sensor for temperature, humidity and CO₂ measurement. |
| `i2c_aht_addr=56` | I²C address of the AHT21 when `SENSOR_ENS160AHT21` is enabled. |
| `i2c_ens_add=83` | I²C address of the ENS160 sensor when `SENSOR_ENS160AHT21` is enabled. |
| `SENSOR_BME280=1` | Use a BME280 sensor for temperature, humidity and pressure. |
| `BME_I2C_ADD=119` | I²C address of the BME280. |
| `I2C_SDA=21` | GPIO pin used as I²C SDA (default 21). |
| `I2C_SCL=22` | GPIO pin used as I²c SCL (default 22). |
| `i2c_pwn_addr=95` | I²C address of the fan PWM controller. |
| `i2c_light_addr=94` | I²C address of the light PWM controller. |
| `time_zone_hour_utc_offset=1` | Offset in hours from UTC for sunrise/sunset calculations. |
| `MYSSID=""` | SSID to connect. |
| `PW=""` | Password for that SSID. |
| `http_port=80` | HTTP port on which the web server listens. |
| `GOVEE_BTH5179=1` | Enable support for a specific device (e.g., GOVEE BTH‑5179). |

**Tip:** If you add new hardware, just create a corresponding flag and guard its code with `#ifdef`.  
After editing **platformio.ini**, rebuild with

Open a terminal in the project root and run:

```bash
pio run -e esp32dev
```

This compiles the firmware defined by **platformio.ini** and all libraries listed under `lib_deps`.

### 3. Flash to ESP‑32

Connect your ESP‑32, identify the serial port (e.g., `/dev/ttyUSB0` on Linux or `COM5` on Windows). Then execute:

```bash
pio run --target upload -e esp32dev -d <serial_port>
```

Alternatively, using the PlatformIO IDE:
1. Click **PlatformIO: Build**.
2. After a successful build, click **Upload**.

### 4. Upload SPIFFS (if needed)

If your firmware uses a filesystem and you want to upload it separately:

```bash
pio run --target uploadfs -e esp32dev
```

```bash
curl.exe -X POST http://192.168.178.143/flashspiffs -F "file=@spiffs.bin"
```

### Notes

- The **platformio.ini** file contains the board definition (`esp32dev`) and library dependencies.
- If you need a larger flash size, uncomment or modify the `board_build.flash_size` setting in **platformio.ini**.

