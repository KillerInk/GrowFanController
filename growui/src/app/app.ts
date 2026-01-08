// growui/src/app/app.ts
import { Component, Signal, signal, ChangeDetectorRef  } from '@angular/core';
import { RouterOutlet } from '@angular/router';
import { ApiService } from './api.service';
import { WebsocketService } from './websocket.service';
import { DeviceState, SocketMsg } from './types';
import { OnInit } from '@angular/core';

@Component({
  selector: 'app-root',
  imports: [RouterOutlet],
  templateUrl: './app.html',
  styleUrl: './app.scss'
})
export class App implements OnInit {

  /* ---------- Dependency Injection ---------- */
  constructor(private api: ApiService, 
    private ws: WebsocketService,
    private cdr: ChangeDetectorRef) {
    this.ws.onMessage().subscribe(msg => this.handleWsMsg(msg));
  }
  fanSettings!: DeviceState;
  socketdata: SocketMsg | null = null;

  ngOnInit(): void {
    this.api.getFanControllerSettings().subscribe(
      (data) => {
        this.fanSettings = data;
        console.log('Fan controller settings loaded:', this.fanSettings);
      },
      (err) => {
        console.error('Failed to load fan settings', err);
      }
    );
  }

  private handleWsMsg(message: string): void {
    try {

      const cleaned = (typeof message === 'string'
        ? message.trim().replace(/^\ufeff/, '')          // strip BOM
        : JSON.stringify(message));
      this.socketdata = JSON.parse(cleaned);   // set plain value
      this.cdr.markForCheck();                // notify Angular

      /* ---------- Optional sensor data ----------
         Add similar blocks for bme280, ens160aht21 as needed. */
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