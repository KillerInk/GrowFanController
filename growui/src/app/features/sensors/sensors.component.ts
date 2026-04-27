// src/app/features/sensors/sensors.component.ts
import { Component, inject, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { DashboardService } from '../../services/dashboard.service';

@Component({
  selector: 'app-sensors',
  imports: [CommonModule],
  templateUrl: './sensors.component.html',
  styleUrl: './sensors.component.scss',
})
export class SensorsComponent {
  private readonly dashboard = inject(DashboardService);

  readonly ds = signal(this.dashboard);

  socketdata() { return this.dashboard.socketdata(); }
  deviceState() { return this.dashboard.deviceState(); }
}
