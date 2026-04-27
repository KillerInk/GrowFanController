/* growui/src/app/app.ts */
import { Component, HostListener, ViewChild, ChangeDetectorRef } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { signal } from '@angular/core';
import { ApiService } from './api.service';
import { WebsocketService } from './websocket.service';
import { ChartComponent } from './chart/chart.component';
import { DeviceState, SocketMsg } from './types';

@Component({
  selector: 'app-root',
  imports: [FormsModule, CommonModule, ChartComponent],
  templateUrl: './app.html',
  styleUrl: './app.scss'
})
export class App {

  readonly deviceState = signal<DeviceState | null>(null);
  readonly socketdata = signal<SocketMsg | null>(null);

  /* ---------- Sidebar state ---------- */
  sidebarOpen = false;
  sidebarMinimized = false;
  activeNav = 'dashboard';
  isMobile = false;

  @HostListener('window:resize')
  onResize() {
    this.isMobile = window.innerWidth < 768;
    if (!this.isMobile) {
      this.sidebarOpen = false;
    }
  }

  toggleSidebar() {
    this.sidebarOpen = !this.sidebarOpen;
  }

  closeSidebar() {
    this.sidebarOpen = false;
  }

  setNav(page: string) {
    this.activeNav = page;
    if (this.isMobile) this.sidebarOpen = false;
  }

  /* ---------- Fan percent signals ---------- */
  readonly fan0percent$ = signal(50);
  readonly fan1percent$ = signal(50);
  readonly cloudSimActive = signal(false);

  selectedFile: File | null = null;
  selectedFileFw: File | null = null;
  @ViewChild('chart') chart?: ChartComponent;

  private wsSubscription?: any;

  /* ---------- Upload progress signals ---------- */
  readonly spiffsUploadPercent = signal(0);
  readonly firmwareUploadPercent = signal(0);

  constructor(protected readonly api: ApiService,
    private ws: WebsocketService,
    private cdr: ChangeDetectorRef) {
    this.api.getFanControllerSettings().subscribe(
      (data) => {
        this.deviceState.set(data);
        this.cloudSimActive.set(data.cloud?.active ?? false);
        this.cdr.markForCheck();
      },
      (err) => {
        console.error('Failed to load fan settings', err);
      }
    );
    this.wsSubscription = this.ws.onMessage().subscribe(msg => this.handleWsMsg(msg));
    this.isMobile = window.innerWidth < 768;
  }

  ngOnDestroy(): void {
    if (this.wsSubscription) this.wsSubscription.unsubscribe();
  }

  private handleWsMsg(message: string): void {
    try {
      const cleaned = (typeof message === 'string'
        ? message.trim().replace(/^\ufeff/, '')
        : JSON.stringify(message));
      this.socketdata.set(JSON.parse(cleaned));
      this.updateFanPercents();
      this.cdr.markForCheck();
      const sd = this.socketdata();
      if (sd) this.chart?.addSocketMessage(sd);
    } catch (e) {
      console.warn('Invalid websocket message', e);
    }
  }

  /* ---------- Slider change handlers (immediate) ---------- */
  onSpeedChange(value: string) {
    this.api.setSpeed(0, Number(value)).subscribe();
  }
  onSpeed1Change(value: string) {
    this.api.setSpeed(1, Number(value)).subscribe();
  }
  onLightChange(value: string) {
    this.api.setLight(Number(value)).subscribe();
  }

  /* ---------- Submit handlers (use signal values) ---------- */
  submitFan0() {
    const state = this.deviceState();
    if (!state) return;
    this.api.setVoltageLimits(0, state.fan0min, state.fan0max).subscribe();
  }
  submitFan1() {
    const state = this.deviceState();
    if (!state) return;
    this.api.setVoltageLimits(1, state.fan1min, state.fan1max).subscribe();
  }

  submitNightMode() {
    const state = this.deviceState();
    if (!state) return;
    this.api.getCmd({
      var: 'fannightmode',
      onh: state.nightmodeonhour,
      onm: state.nightmodeonmin,
      offh: state.nightmodeoffhour,
      offm: state.nightmodeoffmin,
      mspeed: state.nightmodemaxspeed
    }).subscribe();
  }

  submitLightControlVoltage() {
    const state = this.deviceState();
    if (!state) return;
    this.api.getCmd({
      var: 'lightvoltage',
      min: state.lightminvolt,
      max: state.lightmaxvolt
    }).subscribe();
  }

  submitLightControlPercentage() {
    const state = this.deviceState();
    if (!state) return;
    this.api.getCmd({
      var: 'lightlimitsp',
      min: state.lightlimitspmin,
      max: state.lightlimitspmax
    }).subscribe();
  }

  submitCloud() {
    const state = this.deviceState();
    if (!state) return;
    this.api.getCmd({
      var: 'cloudsim',
      cloudduration: state.cloud.cycleduration,
      min: state.cloud.min,
      max: state.cloud.max
    }).subscribe();
  }

  submitCloudActive() {
    const state = this.deviceState();
    if (!state) return;
    this.api.getCmd({
      var: 'cloudsimactive',
      val: state.cloud.active ? 1 : 0
    }).subscribe();
  }

  submitMinMaxSpeed() {
    const state = this.deviceState();
    if (!state) return;
    this.api.setMinMaxSpeed(state.minspeed, state.maxspeed).subscribe();
  }

  submitTargetTempHum() {
    const state = this.deviceState();
    if (!state) return;
    this.api.setTargetTempHum(
      state.targetTemperature,
      state.targetHumidity,
      state.speeddif
    ).subscribe();
  }

  submitTargetTempHumDiff() {
    const state = this.deviceState();
    if (!state) return;
    this.api.setTargetTempHumDiff(state.tempdif ?? 0, state.humdif ?? 0).subscribe();
  }

  submitLightSchedule() {
    const state = this.deviceState();
    if (!state) return;
    this.api.setLightSchedule({
      var: 'lightsettime',
      onh: state.lightonh,
      onmin: state.lightonmin,
      offh: state.lightoffh,
      offmin: state.lightoffmin,
      riseenable: state.lightriseenable ? 1 : 0,
      riseh: state.lightriseh,
      risemin: state.lightrisemin,
      setenable: state.lightsetenable ? 1 : 0,
      seth: state.lightseth,
      setmin: state.lightsetmin
    }).subscribe();
  }

  /* ---------- Toggle handlers ---------- */
  onAutoControlChange(checked: boolean) {
    this.api.setFanAutoControl(checked ? 1 : 0).subscribe();
  }
  onLightAutoChange(checked: boolean) {
    this.api.setLightAutoControl(checked ? 1 : 0).subscribe();
  }
  onNightModeActiveChange(checked: boolean) {
    this.api.setNightModeActive(checked ? 1 : 0).subscribe();
  }
  onReadGoveeChange(checked: boolean) {
    this.api.setReadGovee(checked).subscribe();
  }

  onCloudSimChange(checked: boolean) {
    this.cloudSimActive.set(checked);
    this.api.setCloudSimActive(checked ? 1 : 0).subscribe();
  }

  /* ---------- File upload ---------- */
  onSpiffsFileSelected(event: Event): void {
    const input = event.target as HTMLInputElement;
    if (input.files?.length) this.selectedFile = input.files[0];
  }
  onFirmwareFileSelected(event: Event): void {
    const input = event.target as HTMLInputElement;
    if (input.files?.length) this.selectedFileFw = input.files[0];
  }

  uploadSpiffs(): void {
    const file = this.selectedFile;
    if (!file) { alert('Please choose a file first.'); return; }
    this.api.flashSpiffs(file).subscribe({
      next: (event: any) => {
        if (event.type === 1 && event.total) {
          this.spiffsUploadPercent.set(Math.round(100 * event.loaded / event.total));
        }
      },
      error: err => { console.error('Upload failed', err); alert(`Error: ${err}`); },
      complete: () => this.spiffsUploadPercent.set(0)
    });
  }

  uploadFirmware(): void {
    const file = this.selectedFileFw;
    if (!file) { alert('Please choose a file first.'); return; }
    this.api.flashFirmware(file).subscribe({
      next: (event: any) => {
        if (event.type === 1 && event.total) {
          this.firmwareUploadPercent.set(Math.round(100 * event.loaded / event.total));
        }
      },
      error: err => { console.error(err); alert(`Error: ${err}`); },
      complete: () => this.firmwareUploadPercent.set(0)
    });
  }

  /* ---------- Fan percent calc ---------- */
  private updateFanPercents(): void {
    const state = this.deviceState();
    const sd = this.socketdata();
    if (!state || !sd) return;

    const min0 = state.fan0min, max0 = state.fan0max, cur0 = sd.voltage0;
    if (min0 != null && max0 != null && cur0 != null) {
      const pct0 = Math.round(Math.max(0, Math.min(100, ((cur0 - min0) / (max0 - min0)) * 100)));
      this.fan0percent$.set(pct0);
    }

    const min1 = state.fan1min, max1 = state.fan1max, cur1 = sd.voltage1;
    if (min1 != null && max1 != null && cur1 != null) {
      const pct1 = Math.round(Math.max(0, Math.min(100, ((cur1 - min1) / (max1 - min1)) * 100)));
      this.fan1percent$.set(pct1);
    }
  }
}
