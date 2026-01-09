// growui/src/app/app.ts
import { Component, Signal, signal, ChangeDetectorRef, ViewChild } from '@angular/core';
import { RouterOutlet } from '@angular/router';
import { ApiService } from './api.service';
import { WebsocketService } from './websocket.service';
import { DeviceState, SocketMsg } from './types';
import { OnInit } from '@angular/core';
import { UIChart } from 'primeng/chart';

@Component({
  selector: 'app-root',
  imports: [RouterOutlet, UIChart],
  templateUrl: './app.html',
  styleUrl: './app.scss'
})
export class App implements OnInit {

  /* ---------- Dependency Injection ---------- */
  constructor(private api: ApiService,
    private ws: WebsocketService,
    private cdr: ChangeDetectorRef) {

  }

  fanSettings!: DeviceState;
  socketdata: SocketMsg | null = null;
  @ViewChild('chart')
  chart?: UIChart

  chartOptions?: any = {animation: false,};

  chartData?: any = {};

  private wsSubscription?: any;

  ngOnInit(): void {

    this.chartData = {
      labels: [],
      datasets: []
    };
    this.api.getFanControllerSettings().subscribe(
      (data) => {
        this.fanSettings = data;
        console.log('Fan controller settings loaded:', this.fanSettings);
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

          // Create datasets dynamically
          for (const [key, value] of Object.entries(validFields)) {
            if (key === 'voltage0') {
              this.chartData.datasets.push({
                type: 'line',
                data: [],
                label: 'Fan Voltage',
                backgroundColor: 'rgba(75, 192, 192, 0.2)',
                borderColor: 'rgba(75, 192, 192, 1)',
                tension: 0.3
              });
            } else if (key === 'voltage1') {
              this.chartData.datasets.push({
                type: 'line',
                data: [],
                label: 'Fan2 Voltage',
                backgroundColor: 'rgba(75, 192, 192, 0.2)',
                borderColor: 'rgba(75, 192, 192, 1)',
                tension: 0.3
              });
            } else if (key === 'temperature') {
              this.chartData.datasets.push({
                type: 'line',
                data: [],
                label: 'Temperature (°C)',
                backgroundColor: 'rgba(255, 159, 64, 0.2)',
                borderColor: 'rgba(255, 159, 64, 1)',
                tension: 0.3
              });
            } else if (key === 'humidity') {
              this.chartData.datasets.push({
                type: 'line',
                data: [],
                label: 'Humidity (%)',
                backgroundColor: 'rgba(54, 162, 235, 0.2)',
                borderColor: 'rgba(54, 162, 235, 1)',
                tension: 0.3
              });
            } else if (key === 'co2') {
              this.chartData.datasets.push({
                type: 'line',
                data: [],
                label: 'CO₂ (ppm)',
                backgroundColor: 'rgba(255, 99, 132, 0.2)',
                borderColor: 'rgba(255, 99, 132, 1)',
                tension: 0.3
              });
            } else if (key === 'lightPower') {
              this.chartData.datasets.push({
                type: 'line',
                data: [],
                label: 'Light Power (%)',
                backgroundColor: 'rgba(153, 102, 255, 0.2)',
                borderColor: 'rgba(153, 102, 255, 1)',
                tension: 0.3
              });
            } else if (key === 'lightVoltage') {
              this.chartData.datasets.push({
                type: 'line',
                data: [],
                label: 'Light Voltage (mV)',
                backgroundColor: 'rgba(255, 159, 64, 0.2)',
                borderColor: 'rgba(255, 159, 64, 1)',
                tension: 0.3
              });
            }
          }
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
        this.chartData.datasets[5]?.data?.push(lightP);  // Light Power
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
    } catch (e) {
      console.warn('Invalid websocket message', e);
    }
  }
  /* ---------- Slider change handlers ---------- */
  onSpeedChange(value: string) {                     // <-- changed
    const num = Number(value);                       // convert to number
    this.api.setSpeed(0, num).subscribe();
  }
  onSpeed1Change(value: string) {                    // <-- changed
    const num = Number(value);
    this.api.setSpeed(1, num).subscribe();
  }

  /* ---------- Fan voltage limits (Fan 0 & 1) ---------- */
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

  /* ---------- Night mode settings (example) ---------- */
  submitNightMode() {
    const onHour = Number((document.getElementById('onhour') as HTMLInputElement)?.value);
    const onMin = Number((document.getElementById('onmin') as HTMLInputElement)?.value);
    const offHour = Number((document.getElementById('offhour') as HTMLInputElement)?.value);
    const offMin = Number((document.getElementById('offmin') as HTMLInputElement)?.value);
    const maxSpeed = Number((document.getElementById('nightmodemaxspeed') as HTMLInputElement)?.value);

    this.api.getCmd({
      var: 'nightmode',
      onHour,
      onMin,
      offHour,
      offMin,
      maxSpeed
    }).subscribe();
  }

  /* ---------- Light control (example) ---------- */
  submitLightControlVoltage() {
    const minV = Number((document.getElementById('lightminv') as HTMLInputElement)?.value);
    const maxV = Number((document.getElementById('lightmaxv') as HTMLInputElement)?.value);

    this.api.getCmd({
      var: 'lightvoltage',
      minV,
      maxV
    }).subscribe();
  }

  submitLightControlPercentage() {
    const minP = Number((document.getElementById('lightminp') as HTMLInputElement)?.value);
    const maxP = Number((document.getElementById('lightmaxp') as HTMLInputElement)?.value);

    this.api.getCmd({
      var: 'lightpercentage',
      minP,
      maxP
    }).subscribe();
  }

  /* ---------- Cloud simulation (example) ---------- */
  submitCloud() {
    const cycle = Number((document.getElementById('cloudcycle') as HTMLInputElement)?.value);
    const min = Number((document.getElementById('cloudmin') as HTMLInputElement)?.value);
    const max = Number((document.getElementById('cloudmax') as HTMLInputElement)?.value);

    this.api.getCmd({
      var: 'cloud',
      cycle,
      min,
      max
    }).subscribe();
  }

}