// src/app/services/dashboard.service.ts
import { Injectable, signal } from '@angular/core';
import { WebsocketService } from '../websocket.service';
import { ApiService } from '../api.service';
import { DeviceState, SocketMsg } from '../types';

import { BehaviorSubject } from 'rxjs';

// Lifecycle stage enum
export enum LifecycleStage {
  seedling = 0,
  vegetative = 1,
  flower_early = 2,
  flower_late = 3,
  maturation = 4,
}

export interface LifecycleState {
  enabled: boolean;
  stage: number;
  stageName: string;
  stageDay: number;
  stageStartTimestamp: number;
  accumulatedDLI: number;
  panelMaxPPFD: number;
  umolPerWatt: number;
  currentLightTargetP: number;
}

@Injectable({ providedIn: 'root' })
export class DashboardService {
  readonly deviceState = signal<DeviceState | null>(null);
  readonly socketdata = signal<SocketMsg | null>(null);
  readonly cloudSimActive = signal(false);
  readonly fan0percent$ = signal(50);
  readonly fan1percent$ = signal(50);
  readonly spiffsUploadPercent = signal(0);
  readonly firmwareUploadPercent = signal(0);
  readonly settings$ = new BehaviorSubject<DeviceState | null>(null);

  // Lifecycle state signal
  readonly lifecycleState = signal<LifecycleState | null>(null);

  private _wsSubscription?: any;

  constructor(
    private api: ApiService,
    private ws: WebsocketService,
  ) {
    this._wsSubscription = this.ws.onMessage().subscribe((msg: string) => this.handleWsMsg(msg));

    this.api.getFanControllerSettings().subscribe({
      next: (data: DeviceState) => {
        this.deviceState.set(data);
        this.settings$.next(data);
        this.cloudSimActive.set(data.cloud?.active ?? false);

        // Update lifecycle state from settings
        if (data.lifecycle) {
          this.lifecycleState.set({
            enabled: data.lifecycle.enabled,
            stage: data.lifecycle.stage,
            stageName: data.lifecycle.stageName,
            stageDay: data.lifecycle.stageDay,
            stageStartTimestamp: data.lifecycle.stageStartTimestamp,
            accumulatedDLI: data.lifecycle.accumulatedDLI,
            panelMaxPPFD: data.lifecycle.panelMaxPPFD,
            umolPerWatt: data.lifecycle.umolPerWatt,
            currentLightTargetP: data.lifecycle.currentLightTargetP,
          });
        }
      },
      error: (err: unknown) => console.error('Failed to load fan settings', err),
    });

    // Fetch initial lifecycle state
    this.api.getLifecycleState().subscribe({
      next: (data: any) => {
        if (data?.lifecycle) {
          this.lifecycleState.set({
            enabled: data.lifecycle.enabled,
            stage: data.lifecycle.stage,
            stageName: data.lifecycle.stageName || '',
            stageDay: data.lifecycle.stageDay,
            stageStartTimestamp: 0,
            accumulatedDLI: data.lifecycle.accumulatedDLI,
            panelMaxPPFD: data.lifecycle.panelMaxPPFD,
            umolPerWatt: data.lifecycle.umolPerWatt || 0,
            currentLightTargetP: data.lifecycle.currentLightTargetP,
          });
        }
      },
      error: (err: unknown) => console.error('Failed to load lifecycle state', err),
    });
  }

  private handleWsMsg(message: string): void {
    try {
      const cleaned = (typeof message === 'string'
        ? message.trim().replace(/^\ufeff/, '')
        : JSON.stringify(message));
      const parsed = JSON.parse(cleaned);
      this.socketdata.set(parsed);

      // Update lifecycle state from socket data
      if (parsed.lifecycle) {
        const prev = this.lifecycleState();
        if (!prev || prev.stage !== parsed.lifecycle.stage || prev.enabled !== parsed.lifecycle.enabled) {
          this.lifecycleState.set({
            enabled: parsed.lifecycle.enabled,
            stage: parsed.lifecycle.stage,
            stageName: parsed.lifecycle.stageName || '',
            stageDay: parsed.lifecycle.stageDay,
            stageStartTimestamp: 0, // not in socket
            accumulatedDLI: parsed.lifecycle.accumulatedDLI,
            panelMaxPPFD: parsed.lifecycle.panelMaxPPFD,
            umolPerWatt: 0, // not in socket
            currentLightTargetP: parsed.lifecycle.currentLightTargetP,
          });
        }
      }

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
