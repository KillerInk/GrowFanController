// src/app/features/lights/lights.component.ts
import { Component, inject, effect, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { DashboardService, LifecycleStage } from '../../services/dashboard.service';
import { ApiService } from '../../api.service';

@Component({
  selector: 'app-lights',
  imports: [CommonModule, FormsModule],
  templateUrl: './lights.component.html',
  styleUrl: './lights.component.scss',
  providers: [ApiService],
})
export class LightsComponent {
  private readonly dashboard = inject(DashboardService);
  public readonly api = inject(ApiService);

  readonly ds = this.dashboard;
  readonly apiSignal = this.api;

  deviceState() { return this.dashboard.deviceState(); }
  cloudSimActive() { return this.dashboard.cloudSimActive(); }
  lifecycleState() { return this.dashboard.lifecycleState(); }

  LifecycleStage = LifecycleStage;

  // Temporary bindings synced from signals
  lightMinVolt = signal(0);
  lightMaxVolt = signal(0);
  lightLimitspMin = signal(0);
  lightLimitspMax = signal(0);
  currentLightPct = signal(0);  // Local tracking for slider (percentage)
  lightOnH = signal(0);
  lightOnMin = signal(0);
  lightOffH = signal(0);
  lightOffMin = signal(0);
  lightRiseH = signal(0);
  lightRiseMin = signal(0);
  lightSetH = signal(0);
  lightSetMin = signal(0);
  lightRiseEnable = signal(false);
  lightSetEnable = signal(false);
  cloudCycle = signal(0);
  cloudMin = signal(0);
  cloudMax = signal(0);
  lifecycleStage = signal(0);
  
  // Light state indicators (from WebSocket)
  lightStateName = signal('off');  // 'off', 'on', 'sunrise', 'sunset'
  lightAutoMode = signal(false);   // current automode state from WebSocket

  constructor() {
    // Sync deviceState values to local signals
    effect(() => {
      const state = this.dashboard.deviceState();
      if (state) {
        this.lightMinVolt.set(state.lightminvolt ?? 0);
        this.lightMaxVolt.set(state.lightmaxvolt ?? 0);
        this.lightLimitspMin.set(state.lightlimitspmin ?? 0);
        this.lightLimitspMax.set(state.lightlimitspmax ?? 0);
        this.lightOnH.set(state.lightonh ?? 0);
        this.lightOnMin.set(state.lightonmin ?? 0);
        this.lightOffH.set(state.lightoffh ?? 0);
        this.lightOffMin.set(state.lightoffmin ?? 0);
        this.lightRiseH.set(state.lightriseh ?? 0);
        this.lightRiseMin.set(state.lightrisemin ?? 0);
        this.lightSetH.set(state.lightseth ?? 0);
        this.lightSetMin.set(state.lightsetmin ?? 0);
        this.lightRiseEnable.set(state.lightriseenable ?? false);
        this.lightSetEnable.set(state.lightsetenable ?? false);
        this.cloudCycle.set(state.cloud?.cycleduration ?? 0);
        this.cloudMin.set(state.cloud?.min ?? 0);
        this.cloudMax.set(state.cloud?.max ?? 0);
      }
    });

    // Sync currentLightPct from socket data (lightvalP is the live percentage)
    effect(() => {
      const sd = this.dashboard.socketdata();
      if (sd && sd.lightvalP !== undefined) {
        this.currentLightPct.set(sd.lightvalP);
      }
    });

    // Sync lifecycleState to local signal
    effect(() => {
      const lc = this.dashboard.lifecycleState();
      if (lc) {
        this.lifecycleStage.set(lc.stage);
      }
    });

    // Sync light state indicators from WebSocket (real-time)
    effect(() => {
      const sd = this.dashboard.socketdata();
      if (sd) {
        if (sd.lightStateName) {
          this.lightStateName.set(sd.lightStateName);
        }
        if (sd.lightautomode !== undefined) {
          this.lightAutoMode.set(sd.lightautomode);
        }
      }
    });
  }

  onLightChange(value: string): void {
    const pct = Number(value);
    this.currentLightPct.set(pct);
    // Send percentage directly (0-100) - backend uses getVoltageFromPercent to convert to mV
    this.api.setLight(pct).subscribe();
  }

  submitLightVoltage(): void {
    this.api.getCmd({ var: 'lightvoltage', min: this.lightMinVolt(), max: this.lightMaxVolt() }).subscribe();
  }

  onLightMinVoltChange(value: number): void {
    this.lightMinVolt.set(value);
  }

  onLightMaxVoltChange(value: number): void {
    this.lightMaxVolt.set(value);
  }

  submitLightPercentage(): void {
    this.api.getCmd({ var: 'lightlimitsp', min: this.lightLimitspMin(), max: this.lightLimitspMax() }).subscribe();
  }

  onLightLimitspMinChange(value: number): void {
    this.lightLimitspMin.set(value);
  }

  onLightLimitspMaxChange(value: number): void {
    this.lightLimitspMax.set(value);
  }

  onLightOnHChange(value: number): void { this.lightOnH.set(value); }
  onLightOnMinChange(value: number): void { this.lightOnMin.set(value); }
  onLightOffHChange(value: number): void { this.lightOffH.set(value); }
  onLightOffMinChange(value: number): void { this.lightOffMin.set(value); }
  onLightRiseHChange(value: number): void { this.lightRiseH.set(value); }
  onLightRiseMinChange(value: number): void { this.lightRiseMin.set(value); }
  onLightSetHChange(value: number): void { this.lightSetH.set(value); }
  onLightSetMinChange(value: number): void { this.lightSetMin.set(value); }

  submitCloud(): void {
    this.api.getCmd({
      var: 'cloudsim',
      cloudduration: this.cloudCycle(),
      min: this.cloudMin(),
      max: this.cloudMax(),
    }).subscribe();
  }

  onCloudCycleChange(value: number): void {
    this.cloudCycle.set(value);
  }

  onCloudMinChange(value: number): void {
    this.cloudMin.set(value);
  }

  onCloudMaxChange(value: number): void {
    this.cloudMax.set(value);
  }

  onCloudSimActive(checked: boolean): void {
    this.dashboard.cloudSimActive.set(checked);
    this.api.setCloudSimActive(checked ? 1 : 0).subscribe();
  }

  submitLightSchedule(): void {
    const state = this.dashboard.deviceState();
    if (!state) return;
    this.api.setLightSchedule({
      var: 'lightsettime',
      onh: this.lightOnH(),
      onmin: this.lightOnMin(),
      offh: this.lightOffH(),
      offmin: this.lightOffMin(),
      riseenable: state.lightriseenable ? 1 : 0,
      riseh: this.lightRiseH(),
      risemin: this.lightRiseMin(),
      setenable: state.lightsetenable ? 1 : 0,
      seth: this.lightSetH(),
      setmin: this.lightSetMin(),
    }).subscribe();
  }

  onLightScheduleToggle(checked: boolean, type: 'rise' | 'set'): void {
    if (type === 'rise') {
      this.lightRiseEnable.set(checked);
      // Send both riseenable and lightsetenable to preserve sunset state
      const state = this.dashboard.deviceState();
      const currentSetEnable = state?.lightsetenable ?? false;
      this.api.setLightSchedule({ var: 'lightsettime', onh: 0, onmin: 0, offh: 0, offmin: 0, riseenable: checked ? 1 : 0, riseh: 0, risemin: 0, setenable: currentSetEnable ? 1 : 0, seth: 0, setmin: 0 }).subscribe();
    } else {
      this.lightSetEnable.set(checked);
      // Send both riseenable and lightsetenable to preserve sunrise state
      const state = this.dashboard.deviceState();
      const currentRiseEnable = state?.lightriseenable ?? false;
      this.api.setLightSchedule({ var: 'lightsettime', onh: 0, onmin: 0, offh: 0, offmin: 0, riseenable: currentRiseEnable ? 1 : 0, riseh: 0, risemin: 0, setenable: checked ? 1 : 0, seth: 0, setmin: 0 }).subscribe();
    }
  }

  onLightAuto(checked: boolean): void {
    this.api.setLightAutoControl(checked ? 1 : 0).subscribe();
  }

  // Lifecycle methods
  onLifecycleEnable(checked: boolean): void {
    this.api.setLifecycleEnabled(checked).subscribe();
  }

  onLifecycleStageChange(value: number): void {
    this.lifecycleStage.set(value);
    this.api.setLifecycleStage(value).subscribe();
  }

  onResetLifecycle(): void {
    this.api.resetLifecycle().subscribe();
  }

  onPlantTypeChange(value: number): void {
    this.api.setPlantType(value).subscribe();
    // Update lifecycle state immediately
    const lc = this.dashboard.lifecycleState();
    if (lc) {
      this.dashboard.lifecycleState.set({ ...lc, plantType: value });
    }
  }

  onPanelPPFDChange(value: number): void {
    this.api.setPanelPPFD(value).subscribe();
    // Update local signal immediately so slider doesn't revert
    const lc = this.dashboard.lifecycleState();
    if (lc) {
      this.dashboard.lifecycleState.set({ ...lc, panelMaxPPFD: value });
    }
  }

  // Utility for template
  min(a: number, b: number): number {
    return Math.min(a, b);
  }

  // Get current light percentage (local slider value or device state fallback)
  getLightPct(): number {
    if (this.currentLightPct()) return this.currentLightPct();
    const state = this.deviceState();
    if (!state) return 0;
    const minV = state.lightminvolt ?? 0;
    const maxV = state.lightmaxvolt ?? 10000;
    const voltage = state.lightvalmv ?? 0;
    return Math.round(((voltage - minV) / (maxV - minV)) * 100);
  }

  // Get plant type from lifecycle state (returns 0=photoperiodic by default)
  getPlantType(): number {
    const lc = this.dashboard.lifecycleState();
    return lc?.plantType ?? 0;
  }

  getStageDLI(stage: number): number {
    const dliRange: Record<number, [number, number]> = {
      0: [5, 10],
      1: [10, 16],
      2: [16, 24],
      3: [24, 32],
      4: [32, 40],
    };
    return dliRange[stage]?.[0] ?? 0;
  }

  getStageMaxDLI(stage: number): number {
    const dliRange: Record<number, [number, number]> = {
      0: [5, 10],
      1: [10, 16],
      2: [16, 24],
      3: [24, 32],
      4: [32, 40],
    };
    return dliRange[stage]?.[1] ?? 0;
  }

  // Get human-readable light state (from WebSocket)
  getLightStateLabel(): string {
    const state = this.lightStateName();
    const labels: Record<string, string> = {
      'off': 'Scheduled Off',
      'on': 'Lights ON',
      'sunrise': 'Sunrise Ramp',
      'sunset': 'Sunset Ramp',
    };
    return labels[state] ?? 'Unknown';
  }

  // Check if lights are off due to schedule (automode ON but state is off)
  isScheduledOff(): boolean {
    const automode = this.lightAutoMode() || (this.deviceState()?.lightautomode ?? false);
    const state = this.lightStateName();
    return automode && state === 'off';
  }

  // Get state badge color for UI
  getStateBadgeColor(): string {
    const state = this.lightStateName();
    switch (state) {
      case 'on':
        return 'badge-on';
      case 'sunrise':
        return 'badge-sunrise';
      case 'sunset':
        return 'badge-sunset';
      default:
        return 'badge-off';
    }
  }
}
