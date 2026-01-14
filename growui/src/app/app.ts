// growui/src/app/app.ts
import { Component, Signal, signal, ChangeDetectorRef, ViewChild } from '@angular/core';
import { RouterOutlet } from '@angular/router';
import { ApiService } from './api.service';
import { WebsocketService } from './websocket.service';
import { DeviceState, SocketMsg } from './types';
import { OnInit } from '@angular/core';
import { UIChart } from 'primeng/chart';
import { FormsModule } from '@angular/forms';
import { CommonModule } from '@angular/common';
import 'chartjs-adapter-date-fns';

@Component({
  selector: 'app-root',
  imports: [RouterOutlet, UIChart, FormsModule, CommonModule],
  templateUrl: './app.html',
  styleUrl: './app.scss'
})
export class App implements OnInit {

  /* ---------- Dependency Injection ---------- */
  constructor(private api: ApiService,
    private ws: WebsocketService,
    private cdr: ChangeDetectorRef) {

  }

  deviceState: DeviceState | null = null;
  socketdata: SocketMsg | null = null;
  @ViewChild('chart')
  chart?: UIChart

  chartOptions?: any = {
    animation: false,
    responsive: true,
    maintainAspectRatio: true,
    scales: {
      x: {
        display: true,
        title: { text: 'Time' },
        type: 'time',                     // <-- tell Chart.js the X axis is time
        time: {
          unit: 'second',                 // adjust as needed (second/minute/hour)
          tooltipFormat: 'HH:mm:ss',      // format in tooltips
          displayFormats: {              // format on the axis labels
            second: 'HH:mm:ss',
            minute: 'HH:mm',
            hour: 'HH:mm'
          }
        },
      },
      y: { display: true, title: { text: 'Value' } }
    }
  };

  chartData?: any = {};

  private wsSubscription?: any;

  ngOnInit(): void {

    this.chartData = {
      labels: [],
      datasets: []
    };
    this.api.getFanControllerSettings().subscribe(
      (data) => {
        this.deviceState = data;
        console.log('Fan controller settings loaded:', this.deviceState);
        this.cdr.markForCheck();
      },
      (err) => {
        console.error('Failed to load fan settings', err);
      }
    );
    this.wsSubscription = this.ws.onMessage().subscribe(msg => this.handleWsMsg(msg));
  }

  ngOnDestroy(): void {
    if (this.wsSubscription) {
      this.wsSubscription.unsubscribe();
    }
  }

  private handleWsMsg(message: string): void {
    try {
      const cleaned = (typeof message === 'string'
        ? message.trim().replace(/^\ufeff/, '')          // strip BOM
        : JSON.stringify(message));
      this.socketdata = JSON.parse(cleaned);   // set plain value
      this.cdr.markForCheck();                // notify Angular

      if (this.chart && this.socketdata) {
        // Only process if we have real data
        if (!this.chartData || !this.chartData.labels.length) {
          // First message: initialize chartData based on available fields
          const availableFields = {
            voltage0: this.socketdata.voltage0,
            voltage1: this.socketdata.voltage1,
            temperature: this.socketdata.bme280?.temperatur,
            humidity: this.socketdata.bme280?.humidity,
            co2: this.socketdata.ens160aht21?.eco2,
            lightPower: this.socketdata.lightvalP,
            lightVoltage: this.socketdata.lightvalmv,
            time: this.socketdata.time
          };

          // Filter only non-zero, non-undefined values
          const validFields = Object.fromEntries(
            Object.entries(availableFields).filter(([_, value]) => value !== undefined && value !== null)
          );

          // If no valid fields, skip
          if (Object.keys(validFields).length === 0) {
            return;
          }

          // Initialize chartData with dynamic datasets
          this.chartData = {
            labels: [],
            datasets: []
          };

          const yAxisIds: Record<string, string> = {
            voltage0: 'yVoltage0',
            voltage1: 'yVoltage1',
            temperature: 'yTemperature',
            humidity: 'yHumidity',
            co2: 'yCO2',
            lightPower: 'yLightPower',
            lightVoltage: 'yLightVoltage'
          };

          for (const [key, value] of Object.entries(validFields)) {
            const commonOpts = {
              type: 'line',
              data: [],
              tension: 0.3,
              //yAxisID: yAxisIds[key as keyof typeof yAxisIds]   // unique Y‑axis
            };

            if (key === 'voltage0') {
              this.chartData.datasets.push({
                ...commonOpts,
                label: 'Fan Voltage',
                backgroundColor: 'rgba(75, 192, 192, 0.2)',
                borderColor: 'rgba(75, 192, 192, 1)'
              });
            } else if (key === 'voltage1') {
              this.chartData.datasets.push({
                ...commonOpts,
                label: 'Fan2 Voltage',
                backgroundColor: 'rgba(75, 192, 192, 0.2)',
                borderColor: 'rgba(75, 192, 192, 1)'
              });
            } else if (key === 'temperature') {
              this.chartData.datasets.push({
                ...commonOpts,
                label: 'Temperature (°C)',
                backgroundColor: 'rgba(255, 159, 64, 0.2)',
                borderColor: 'rgba(255, 159, 64, 1)'
              });
            } else if (key === 'humidity') {
              this.chartData.datasets.push({
                ...commonOpts,
                label: 'Humidity (%)',
                backgroundColor: 'rgba(54, 162, 235, 0.2)',
                borderColor: 'rgba(54, 162, 235, 1)'
              });
            } else if (key === 'co2') {
              this.chartData.datasets.push({
                ...commonOpts,
                label: 'CO₂ (ppm)',
                backgroundColor: 'rgba(255, 99, 132, 0.2)',
                borderColor: 'rgba(255, 99, 132, 1)'
              });
            } else if (key === 'lightPower') {
              this.chartData.datasets.push({
                ...commonOpts,
                label: 'Light Power (%)',
                backgroundColor: 'rgba(153, 102, 255, 0.2)',
                borderColor: 'rgba(153, 102, 255, 1)'
              });
            } else if (key === 'lightVoltage') {
              this.chartData.datasets.push({
                ...commonOpts,
                label: 'Light Voltage (mV)',
                backgroundColor: 'rgba(255, 159, 64, 0.2)',
                borderColor: 'rgba(255, 159, 64, 1)'
              });
            }

            // Now start adding data points (first message)
            const timeLabel = Date.now();
            const fan1Val = this.socketdata.voltage0 ?? 0;
            const fan2Val = this.socketdata.voltage1 ?? 0;
            const temp = this.socketdata.bme280?.temperatur ?? 0;
            const hum = this.socketdata.bme280?.humidity ?? 0;
            const co2 = this.socketdata.ens160aht21?.eco2 ?? 0;
            const lightP = this.socketdata.lightvalP ?? 0;
            const lightV = this.socketdata.lightvalmv ?? 0;

            // Push all values
            this.chartData.labels.push(timeLabel);
            this.chartData.datasets[0]?.data?.push(fan1Val); // Fan Voltage
            this.chartData.datasets[1]?.data?.push(fan2Val); // Fan2 Voltage
            this.chartData.datasets[2]?.data?.push(temp);     // Temperature
            this.chartData.datasets[3]?.data?.push(hum);     // Humidity
            this.chartData.datasets[4]?.data?.push(co2);     // CO2
            this.chartData.datasets[5]?.data?.push(lightP);  // Light %
            this.chartData.datasets[6]?.data?.push(lightV);  // Light Voltage

            // Keep only the last 50 points
            /*if (this.chartData.labels.length > 50) {
              this.chartData.labels.shift();
              this.chartData.datasets[0]?.data?.shift();
              this.chartData.datasets[1]?.data?.shift();
              this.chartData.datasets[2]?.data?.shift();
              this.chartData.datasets[3]?.data?.shift();
              this.chartData.datasets[4]?.data?.shift();
              this.chartData.datasets[5]?.data?.shift();
              this.chartData.datasets[6]?.data?.shift();
            }*/
            this.chart.chart.update();

          }
        }
      }
    } catch (e) {
      console.warn('Invalid websocket message', e);
    }
  }
  /* ---------- Slider change handlers ---------- */
  onSpeedChange(value: string) {
    const num = Number(value);
    this.api.setSpeed(0, num).subscribe();
  }
  onSpeed1Change(value: string) {
    const num = Number(value);
    this.api.setSpeed(1, num).subscribe();
  }

  onLightChange(value: string) {
    const num = Number(value);
    this.api.setLight(num).subscribe();
  }

  submitFan0() {
    const min = Number((document.getElementById('fan0min') as HTMLInputElement)?.value);
    const max = Number((document.getElementById('fan0max') as HTMLInputElement)?.value);
    this.api.setVoltageLimits(0, min, max).subscribe();
  }
  submitFan1() {
    const min = Number((document.getElementById('fan1min') as HTMLInputElement)?.value);
    const max = Number((document.getElementById('fan1max') as HTMLInputElement)?.value);
    this.api.setVoltageLimits(1, min, max).subscribe();
  }

  submitNightMode() {
    const onh = Number((document.getElementById('onhour') as HTMLInputElement)?.value);
    const onm = Number((document.getElementById('onmin') as HTMLInputElement)?.value);
    const offh = Number((document.getElementById('offhour') as HTMLInputElement)?.value);
    const offm = Number((document.getElementById('offmin') as HTMLInputElement)?.value);
    const mspeed = Number((document.getElementById('nightmodemaxspeed') as HTMLInputElement)?.value);

    this.api.getCmd({
      var: 'fannightmode',
      onh,
      onm,
      offh,
      offm,
      mspeed
    }).subscribe();
  }

  submitLightControlVoltage() {
    const min = Number((document.getElementById('lightminv') as HTMLInputElement)?.value);
    const max = Number((document.getElementById('lightmaxv') as HTMLInputElement)?.value);

    this.api.getCmd({
      var: 'lightvoltage',
      min,
      max
    }).subscribe();
  }

  submitLightControlPercentage() {
    const min = Number((document.getElementById('lightminp') as HTMLInputElement)?.value);
    const max = Number((document.getElementById('lightmaxp') as HTMLInputElement)?.value);

    this.api.getCmd({
      var: 'lightlimitsp',
      min,
      max
    }).subscribe();
  }

  submitCloud() {
    const cloudduration = Number((document.getElementById('cloudcycle') as HTMLInputElement)?.value);
    const min = Number((document.getElementById('cloudmin') as HTMLInputElement)?.value);
    const max = Number((document.getElementById('cloudmax') as HTMLInputElement)?.value);

    this.api.getCmd({
      var: 'cloudsim',
      cloudduration,
      min,
      max
    }).subscribe();
  }

  submitMinMaxSpeed() {
    const min = Number((document.getElementById('minspeed') as HTMLInputElement)?.value);
    const max = Number((document.getElementById('maxspeed') as HTMLInputElement)?.value);

    this.api.setMinMaxSpeed(min, max).subscribe();
  }

  submitTargetTempHum() {
    const temp = Number((document.getElementById('targettemp') as HTMLInputElement)?.value);
    const hum = Number((document.getElementById('targethum') as HTMLInputElement)?.value);
    const speeddif = Number((document.getElementById('speeddif') as HTMLInputElement)?.value);

    this.api.setTargetTempHum(temp, hum, speeddif).subscribe();
  }

  onLightAutoChange(checked: boolean) {
    this.api.setLightAutoControl(checked ? 1 : 0).subscribe();
  }

  onNightModeActiveChange(checked: boolean) {
    this.api.setNightModeActive(checked ? 1 : 0).subscribe();
  }

  onAutoControlChange(checked: boolean) {
    this.api.setFanAutoControl(checked ? 1 : 0).subscribe();
  }

  submitLightSchedule() {
    const onh = Number((document.getElementById('turnlightonhour') as HTMLInputElement)?.value);
    const onmin = Number((document.getElementById('turnlightonmin') as HTMLInputElement)?.value);
    const offh = Number((document.getElementById('turnlightoffhour') as HTMLInputElement)?.value);
    const offmin = Number((document.getElementById('turnlightoffmin') as HTMLInputElement)?.value);

    const riseenable = (document.getElementById('enablesunrise') as HTMLInputElement).checked ? 1 : 0;
    const riseh = Number((document.getElementById('sunrisehour') as HTMLInputElement)?.value);
    const risemin = Number((document.getElementById('sunrisemin') as HTMLInputElement)?.value);

    const setenable = (document.getElementById('enablesunset') as HTMLInputElement).checked ? 1 : 0;
    const seth = Number((document.getElementById('sunsethour') as HTMLInputElement)?.value);
    const setmin = Number((document.getElementById('sunsetmin') as HTMLInputElement)?.value);

    this.api.setLightSchedule({
      var: 'lightsettime',
      onh,
      onmin,
      offh,
      offmin,
      riseenable,
      riseh,
      risemin,
      setenable,
      seth,
      setmin
    }).subscribe();
  }

}