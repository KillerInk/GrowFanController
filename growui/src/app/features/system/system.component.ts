// src/app/features/system/system.component.ts
import { Component, inject, signal, OnInit, OnDestroy } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { HttpEventType } from '@angular/common/http';
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

  // Timezone state
  timezoneOffset = signal(0);
  editingTimezone = signal(false);
  timezoneInput = signal(0);

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
      this.timezoneOffset.set(state.timezoneOffset ?? 0);
    });
    // Load timezone from device
    this.api.getTimeZone().subscribe({
      next: (res: any) => {
        if (res?.offset != null) this.timezoneOffset.set(res.offset);
      },
      error: () => {
        // fallback to device state value
      },
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
        if (evt.type === HttpEventType.UploadProgress) {
          const percent = Math.round((evt.loaded / evt.total) * 100);
          this.dashboard.spiffsUploadPercent.set(percent);
        }
      },
      complete: () => {
        // Reload with cache-busting to load fresh Angular files after SPIFFS update
        this.reloadWithCacheBust();
      },
    });
  }

  onFirmwareFileChange(event: Event): void {
    const file = (event.target as HTMLInputElement).files?.[0];
    if (!file) return;
    this.api.uploadFirmware(file).subscribe({
      next: (evt: any) => {
        if (evt.type === HttpEventType.UploadProgress) {
          const percent = Math.round((evt.loaded / evt.total) * 100);
          this.dashboard.firmwareUploadPercent.set(percent);
        }
      },
      complete: () => {
        // Reload with cache-busting to load fresh Angular files after firmware update
        this.reloadWithCacheBust();
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
    
    console.log('Saving WiFi credentials:', { ssid, password, apFallback: this.apFallbackEnabled(), timeout: this.connectTimeout() });
    
    this.api.setWifiCredentials(ssid, password, this.apFallbackEnabled(), this.connectTimeout()).subscribe({
      next: (res: any) => {
        console.log('WiFi config response:', res);
        // Device will reboot, no response expected
      },
      error: (err: any) => {
        console.error('WiFi config failed:', err);
      },
      complete: () => {
        console.log('WiFi config complete');
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

  /* ---------- Timezone ---------- */

  onEditTimezone(): void {
    this.timezoneInput.set(this.timezoneOffset());
    this.editingTimezone.set(true);
  }

  onSaveTimezone(): void {
    const offset = this.timezoneInput();
    this.api.setTimeZone(offset).subscribe({
      next: () => {
        this.timezoneOffset.set(offset);
        this.editingTimezone.set(false);
      },
      error: (err: any) => {
        console.error('Timezone save failed:', err);
      },
    });
  }

  onCancelTimezone(): void {
    this.editingTimezone.set(false);
  }

  onToggleTimezoneEdit(): void {
    this.editingTimezone.set(!this.editingTimezone());
  }

  /** Reload the page with a cache-busting timestamp to force fresh resources */
  reloadWithCacheBust(): void {
    const t = Date.now();
    window.location.href = window.location.origin + '?t=' + t;
  }
}
