export interface FanValues {
  fan0voltage: number;
  fan0min: number;
  fan0max: number;
  fan1voltage: number;
  fan1min: number;
  fan1max: number;

  autocontrol: boolean;
  targetTemperature: number;
  targetHumidity: number;
  readgovee: boolean;
  speeddif: number;          // filter compensation

  tempdif?: number;          // optional – depends on sensor
  humdif?: number;           // optional – depends on sensor

  minspeed: number;
  maxspeed: number;

  nightmodeactive: boolean;
  nightmodeonhour: number;
  nightmodeonmin: number;
  nightmodeoffmin: number;
  nightmodeoffhour: number;
  nightmodemaxspeed: number;
}

export interface LifecycleConfig {
  enabled: boolean;
  stage: number;             // lifecycle_stage enum (0-4)
  stageName: string;         // human-readable name
  stageDay: number;          // day within current stage (1-based)
  stageStartTimestamp: number;  // epoch timestamp
  accumulatedDLI: number;    // cumulative DLI (μmol/m²/day)
  panelMaxPPFD: number;      // panel max PPFD (μmol/m²/s)
  umolPerWatt: number;       // panel efficiency (μmol/J)
  currentLightTargetP: number;  // computed light target %
}

export interface LightValues {
  lightonh: number;
  lightonmin: number;
  lightoffh: number;
  lightoffmin: number;
  lightriseh: number;
  lightrisemin: number;
  lightseth: number;
  lightsetmin: number;
  lightriseenable: boolean;
  lightsetenable: boolean;
  lightautomode: boolean;

  lightminvolt: number;
  lightmaxvolt: number;
  lightlimitspmin: number;
  lightlimitspmax: number;

  cloud: {
    active: boolean;
    cycleduration: number;   // in minutes
    min: number;             // min_light_cloudP
    max: number;             // max_light_cloudP
  };

  // Lifecycle fields (added 2026-04-10)
  lifecycle: LifecycleConfig;
}

export interface DeviceState extends FanValues, LightValues {
  /** Light value in millivolts (also in SocketMsg) */
  lightvalmv?: number;

  /** System info fields */
  uptime?: string;
  freeHeap?: number;
  minFreeHeap?: number;
  maxAlloc?: number;
  chipid?: string;
  firmwareVersion?: string;
  chipmodel?: string;
  spisize?: number;
  spiflashspeed?: number;
  spiflsize?: number;

  /** WiFi status fields */
  wifi_connected?: boolean;
  wifi_rssi?: number;
  wifi_ssid?: string;
  ap_active?: boolean;
  ap_ssid?: string;
  ap_ip?: string;

  /** Timezone offset in hours from UTC */
  timezoneOffset?: number;
}

//{\"bme280\":{
// "temperatur":"21.72",
// "humidity":"33.12",
// "atemperatur":"21.72",
// "ahumidity":"33.12",
// "pressure":"952.54",
// "apressure":"952.56"
// },
// "voltage0":573,
// "voltage1":671,
// "nightmode":false,
// "time":"14:08:45",
// "lightvalP":0,
// "lightvalmv":0,
// "lightstate":0,
// "vpdair":"1.74"}
export interface SocketMsg {

  govee?:
  {
    temperatur: string;
    humidity: string;
    battery: string;
  }
  /** Optional BME280 sensor data */
  bme280?: {
    temperatur: string;   // e.g. "23.45"
    humidity: string;
    atemperatur: string;   // average temperature
    ahumidity: string;     // average humidity
    pressure: string;
    apressure: string;      // average pressure
    vpd: string;
  };

  /** Optional ENS160AHT21 sensor data */
  ens160aht21?: {
    temperatur: string;
    humidity: string;
    atemperatur: string;
    ahumidity: string;
    eco2: string;   // CO₂ value
    aqi: string;    // Air Quality Index
    tvoc: string;   // Total Volatile Organic Compounds
    vpd: string;
  };

  /** VPD as number for comparisons */
  vpdNum?: number;

  /** Fan controller values */
  autocontrolspeed: boolean;      // only if auto‑control enabled
  voltage0: number;
  voltage1: number;
  nightmode: boolean;

  /** Current time in HH:MM:SS format */
  time: string;

  /** Light controller values */
  lightvalP: number;     // current light power (in %?)
  lightvalmv: number;    // voltage value
  lightstate: number;    // current state of the light

  /** Lifecycle/PPFD/DLI data (added 2026-04-10) */
  lifecycle?: {
    enabled: boolean;
    stage: number;
    stageName: string;
    stageDay: number;
    currentLightTargetP: number;
    panelMaxPPFD: number;
    accumulatedDLI: number;
  };

  vpdair: string;        // VPD (if SENSOR_BME280 defined)
}
