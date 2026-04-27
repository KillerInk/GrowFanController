// src/app/services/dashboard.service.ts
import { Injectable, signal } from '@angular/core';
import { WebsocketService } from '../websocket.service';
import { ApiService } from '../api.service';
import { DeviceState, SocketMsg } from '../types';

@Injectable({ providedIn: 'root' })
export class DashboardService {
  readonly deviceState = signal<DeviceState | null>(null);
  readonly socketdata = signal<SocketMsg | null>(null);
  readonly cloudSimActive = signal(false);
  readonly fan0percent$ = signal(50);
  readonly fan1percent$ = signal(50);
  readonly spiffsUploadPercent = signal(0);
  readonly firmwareUploadPercent = signal(0);

  private _wsSubscription?: any;

  constructor(
    private api: ApiService,
    private ws: WebsocketService,
  ) {
    this._wsSubscription = this.ws.onMessage().subscribe((msg: string) => this.handleWsMsg(msg));

    this.api.getFanControllerSettings().subscribe({
      next: (data: DeviceState) => {
        this.deviceState.set(data);
        this.cloudSimActive.set(data.cloud?.active ?? false);
      },
      error: (err: unknown) => console.error('Failed to load fan settings', err),
    });
  }

  private handleWsMsg(message: string): void {
    try {
      const cleaned = (typeof message === 'string'
        ? message.trim().replace(/^\ufeff/, '')
        : JSON.stringify(message));
      this.socketdata.set(JSON.parse(cleaned));
      this.updateFanPercents();
    } catch (e) {
      console.warn('Invalid websocket message', e);
    }
  }

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
