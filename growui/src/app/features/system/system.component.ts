// src/app/features/system/system.component.ts
import { Component, inject, signal, OnInit, OnDestroy } from '@angular/core';
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
export class SystemComponent implements OnInit, OnDestroy {
  private readonly dashboard = inject(DashboardService);
  public readonly api = inject(ApiService);

  readonly ds = signal(this.dashboard);
  readonly apiSignal = signal(this.api);

  deviceState() { return this.dashboard.deviceState(); }

  // WiFi config state
  wifiSsid = signal('');
  wifiConnected = signal(false);
  wifiRssi = signal(0);
  apActive = signal(false);
  apSsid = signal('');
  apIp = signal('');
  apFallbackEnabled = signal(true);
  connectTimeout = signal(15);

  // Local form state
  newSsid = signal('');
  newPassword = signal('');

  private settingsSubscription: any = null;

  ngOnInit(): void {
    // Subscribe to settings updates (includes WiFi status)
    this.settingsSubscription = this.dashboard.settings$.subscribe((state: any) => {
      if (!state) return;
      this.wifiSsid.set(state.wifi_ssid || '');
      this.wifiConnected.set(!!state.wifi_connected);
      this.wifiRssi.set(state.wifi_rssi || 0);
      this.apActive.set(!!state.ap_active);
      this.apSsid.set(state.ap_ssid || '');
      this.apIp.set(state.ap_ip || '');
    });
  }

  ngOnDestroy(): void {
    if (this.settingsSubscription) {
      this.settingsSubscription.unsubscribe();
    }
  }

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

  onSaveWifiCredentials(): void {
    const ssid = this.newSsid().trim();
    const password = this.newPassword().trim();
    if (!ssid) return;
    
    this.api.setWifiCredentials(ssid, password, this.apFallbackEnabled(), this.connectTimeout()).subscribe({
      complete: () => {
        // Device will reboot, no response expected
      },
      error: (err: any) => {
        console.error('WiFi config failed:', err);
      },
    });
  }

  onToggleApFallback(): void {
    this.apFallbackEnabled.set(!this.apFallbackEnabled());
  }

  onTimeoutChange(): void {
    const val = parseInt(this.connectTimeout().toString(), 10);
    if (val < 1 || val > 60) {
      this.connectTimeout.set(15);
    }
  }
}
