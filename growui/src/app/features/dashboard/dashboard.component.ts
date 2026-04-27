// src/app/features/dashboard/dashboard.component.ts
import { Component, ViewChild, ElementRef, inject, AfterViewInit, OnDestroy, effect, untracked } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { DashboardService } from '../../services/dashboard.service';
import { ApiService } from '../../api.service';
import { ChartComponent } from '../../chart/chart.component';
import { SocketMsg } from '../../types';

@Component({
  selector: 'app-dashboard',
  imports: [CommonModule, FormsModule, ChartComponent],
  templateUrl: './dashboard.component.html',
  styleUrl: './dashboard.component.scss',
  providers: [ApiService],
})
export class DashboardComponent implements AfterViewInit, OnDestroy {
  private readonly dashboard = inject(DashboardService);
  public readonly api = inject(ApiService);

  readonly ds = this.dashboard;
  readonly apiSignal = this.api;

  parseFloat = parseFloat;

  @ViewChild(ChartComponent) chartRef?: ChartComponent;
  private _chart?: ChartComponent;

  constructor() {
    // Track socketdata signal and forward to chart when it changes
    effect(() => {
      const socketData = this.dashboard.socketdata();
      if (socketData && this._chart) {
        untracked(() => {
          this._chart?.addSocketMessage(socketData);
        });
      }
    });
  }

  ngAfterViewInit(): void {
    // Delay chart assignment to ensure it's resolved
    setTimeout(() => {
      this._chart = this.chartRef;
    }, 0);
  }

  ngOnDestroy(): void {
  }

  socketdata() { return this.dashboard.socketdata(); }
  deviceState() { return this.dashboard.deviceState(); }
}
