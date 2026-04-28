// src/app/features/fans/fans.component.ts
import { Component, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { DashboardService } from '../../services/dashboard.service';
import { ApiService } from '../../api.service';

@Component({
  selector: 'app-fans',
  imports: [CommonModule, FormsModule],
  templateUrl: './fans.component.html',
  styleUrl: './fans.component.scss',
  providers: [ApiService],
})
export class FansComponent {
  private readonly dashboard = inject(DashboardService);
  public readonly api = inject(ApiService);

  readonly ds = this.dashboard;
  readonly apiSignal = this.api;

  deviceState() { return this.dashboard.deviceState(); }

  getFanPercent(id: number): number {
    const state = this.deviceState();
    if (!state) return 0;
    
    const voltage = id === 0 ? state.fan0voltage : state.fan1voltage;
    const min = id === 0 ? state.fan0min : state.fan1min;
    const max = id === 0 ? state.fan0max : state.fan1max;
    
    if (max === min || voltage == null || min == null || max == null) return 0;
    
    return Math.round(Math.max(0, Math.min(100, ((voltage - min) / (max - min)) * 100)));
  }

  onSpeedChange(id: number, value: string): void {
    this.api.setSpeed(id, Number(value)).subscribe();
  }

  submitFan(id: number): void {
    const state = this.dashboard.deviceState();
    if (!state) return;
    if (id === 0) {
      this.api.setVoltageLimits(0, state.fan0min, state.fan0max).subscribe();
    } else {
      this.api.setVoltageLimits(1, state.fan1min, state.fan1max).subscribe();
    }
  }

  submitMinMaxSpeed(): void {
    const state = this.dashboard.deviceState();
    if (!state) return;
    this.api.setMinMaxSpeed(state.minspeed, state.maxspeed).subscribe();
  }

  submitNightMode(): void {
    const state = this.dashboard.deviceState();
    if (!state) return;
    this.api.getCmd({
      var: 'fannightmode',
      onh: state.nightmodeonhour,
      onm: state.nightmodeonmin,
      offh: state.nightmodeoffhour,
      offm: state.nightmodeoffmin,
      mspeed: state.nightmodemaxspeed,
    }).subscribe();
  }

  onAutoControl(checked: boolean): void {
    this.api.setFanAutoControl(checked ? 1 : 0).subscribe();
  }

  onNightModeActive(checked: boolean): void {
    this.api.setNightModeActive(checked ? 1 : 0).subscribe();
  }

  onReadGovee(checked: boolean): void {
    this.api.setReadGovee(checked).subscribe();
  }

  submitTargetTempHum(): void {
    const state = this.dashboard.deviceState();
    if (!state) return;
    this.api.setTargetTempHum(state.targetTemperature, state.targetHumidity, state.speeddif).subscribe();
  }
}
