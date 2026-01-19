// growui/src/app/app.ts
import { Component, Signal, signal, ChangeDetectorRef, ViewChild, AfterViewInit } from '@angular/core';
import { RouterOutlet } from '@angular/router';
import { ApiService } from './api.service';
import { WebsocketService } from './websocket.service';
import { DeviceState, SocketMsg } from './types';
import { OnInit } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { CommonModule } from '@angular/common';
import { ChartComponent } from './chart/chart.component';

@Component({
  selector: 'app-root',
  imports: [RouterOutlet, FormsModule, CommonModule,ChartComponent],
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

  selectedFile: File | null = null;
  selectedFileFw: File | null = null;
  @ViewChild('chart') chart?: ChartComponent;

  private wsSubscription?: any;

  ngOnInit(): void {
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
      if(this.socketdata)
        this.chart?.addSocketMessage(this.socketdata);
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

  submitTargetTempHumDiff() {
    const temp = Number((document.getElementById('tempdif') as HTMLInputElement)?.value);
    const hum = Number((document.getElementById('humdif') as HTMLInputElement)?.value);

    this.api.setTargetTempHumDiff(temp, hum).subscribe();
  }

  onReadGoveeChange(checked: boolean): void {
    this.api.setReadGovee(checked).subscribe(
      () => console.log('ReadGovee setting updated', checked),
      err => console.error('Failed to update ReadGovee:', err)
    );
  }

  onSpiffsFileSelected(event: Event): void {
    const input = event.target as HTMLInputElement;
    if (input.files && input.files.length) {
      this.selectedFile = input.files[0];
      console.log('Chosen file:', this.selectedFile.name);
    }
  }

  onFirmwareFileSelected(event: Event): void {
    const input = event.target as HTMLInputElement;
    if (input.files && input.files.length) {
      this.selectedFileFw = input.files[0];
      console.log('Chosen file:', this.selectedFileFw.name);
    }
  }

  /* ---------- Upload method ----------
   * Sends the selected file to `/flashspiffs`
   */
  uploadSpiffs(): void {
    if (!this.selectedFile) {
      alert('Please choose a file first.');
      return;
    }

    // Call ApiService.flashSpiffs
    this.api.flashSpiffs(this.selectedFile).subscribe({
      next: msg => {
        console.log('Upload succeeded:', msg);
        //alert(msg);   // optional user feedback
      },
      error: err => {
        console.error('Upload failed', err);
        alert(`Error: ${err}`);
      }
    });
  }

  uploadFirmware(): void {
    if (!this.selectedFileFw) {
        alert('Please choose a file first.');
        return;
    }

    this.api.flashFirmware(this.selectedFileFw).subscribe({
      next: msg => console.log('Firmware upload succeeded:', msg),
      error: err => { console.error(err); alert(`Error: ${err}`); }
    });
}
}