// src/app/features/lights/lights.component.ts
import { Component, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { DashboardService } from '../../services/dashboard.service';
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

  onLightChange(value: string): void {
    this.api.setLight(Number(value)).subscribe();
  }

  submitLightVoltage(): void {
    const state = this.dashboard.deviceState();
    if (!state) return;
    this.api.getCmd({ var: 'lightvoltage', min: state.lightminvolt, max: state.lightmaxvolt }).subscribe();
  }

  submitLightPercentage(): void {
    const state = this.dashboard.deviceState();
    if (!state) return;
    this.api.getCmd({ var: 'lightlimitsp', min: state.lightlimitspmin, max: state.lightlimitspmax }).subscribe();
  }

  submitCloud(): void {
    const state = this.dashboard.deviceState();
    if (!state) return;
    this.api.getCmd({
      var: 'cloudsim',
      cloudduration: state.cloud?.cycleduration ?? 0,
      min: state.cloud?.min ?? 0,
      max: state.cloud?.max ?? 0,
    }).subscribe();
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
      onh: state.lightonh,
      onmin: state.lightonmin,
      offh: state.lightoffh,
      offmin: state.lightoffmin,
      riseenable: state.lightriseenable ? 1 : 0,
      riseh: state.lightriseh,
      risemin: state.lightrisemin,
      setenable: state.lightsetenable ? 1 : 0,
      seth: state.lightseth,
      setmin: state.lightsetmin,
    }).subscribe();
  }

  onLightAuto(checked: boolean): void {
    this.api.setLightAutoControl(checked ? 1 : 0).subscribe();
  }
}
