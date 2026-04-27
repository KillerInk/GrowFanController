// src/app/features/system/system.component.ts
import { Component, inject, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { DashboardService } from '../../services/dashboard.service';
import { ApiService } from '../../api.service';

@Component({
  selector: 'app-system',
  imports: [CommonModule, FormsModule],
  templateUrl: './system.component.html',
  styleUrl: './system.component.scss',
  providers: [ApiService],
})
export class SystemComponent {
  private readonly dashboard = inject(DashboardService);
  public readonly api = inject(ApiService);

  readonly ds = signal(this.dashboard);
  readonly apiSignal = signal(this.api);

  deviceState() { return this.dashboard.deviceState(); }

  onSpiffsFileChange(event: Event): void {
    const file = (event.target as HTMLInputElement).files?.[0];
    if (!file) return;
    this.api.uploadSpiffs(file).subscribe({
      next: (evt: any) => {
        if (evt?.progress) this.dashboard.spiffsUploadPercent.set(evt.progress);
      },
    });
  }

  onFirmwareFileChange(event: Event): void {
    const file = (event.target as HTMLInputElement).files?.[0];
    if (!file) return;
    this.api.uploadFirmware(file).subscribe({
      next: (evt: any) => {
        if (evt?.progress) this.dashboard.firmwareUploadPercent.set(evt.progress);
      },
    });
  }

  onReset(soft: boolean): void {
    this.api.getCmd({ var: soft ? 'softreset' : 'hardreset' }).subscribe();
  }
}
