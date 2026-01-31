/* growui/src/app/chart/chart.component.ts */
import { Component, ElementRef, Input, ViewChild } from '@angular/core';
import { UIChart } from 'primeng/chart';
import { SocketMsg } from '../types';
import 'chartjs-adapter-date-fns';
import { CommonModule } from '@angular/common';
import { Chart } from 'chart.js';
import { LegendItem } from 'chart.js';
import { ApiService } from '../api.service';
import { tap } from 'rxjs/operators';
import { firstValueFrom, range } from 'rxjs';
import { chartOptionsBase, colors, csv_yAxisIds, yAxisIds } from './chart-config';
import { createSampledData } from './chart-sampledata';
import { restoreDatasetVisibility, saveDatasetVisibility } from './chart-visibility';
import { initializeDatasets } from './chart-initdatasets';
import { loadHistoricalData } from './chart-historyloading';

@Component({
  selector: 'app-chart',
  imports: [UIChart, CommonModule],
  styleUrl: "./chart.component.scss",
  templateUrl: './chart.component.html',
})
export class ChartComponent {
  constructor(private apiService: ApiService) { }
  @ViewChild('chart') chart?: UIChart;
  @ViewChild('chartContainer') chartContainer?: ElementRef;

  private visibleItemCount = 0;
  private itemPosition = 0;
  private mousebuttonpressed = false;
  private mousestartposition = 0;
  public isMouseOverChart = false;

  private loadedHours: Set<string> = new Set();
  /* Flag to prevent overlapping load requests */
  private loadingPreviousHour = false;

  private datasetKeyIndexMap: Record<string, number> = {};
  /** Chart configuration – can be set externally if needed */
  chartOptions: any = {
    ...chartOptionsBase,
    plugins: {
      legend: { display: true, onClick: (e: any, legendItem: any, legend: any) => this.onLegendClick(e, legendItem, legend) },
    },
  };

  /** Internal chart data – initialized on first message */
  chartData: any = { labels: [], datasets: [] };
  private initialized = false;

  private fullChartData: { labels: number[]; datasets: any[] } = {
    labels: [],
    datasets: []
  };

  private currentRange: '10min' | '30min' | '1h' | '2h' | '4h' = '10min';

  private onLegendClick(e: any, legendItem: any, legend: any) {
    const ci = legend.chart;
    const datasetIndex = legendItem.datasetIndex;


    // Toggle visibility
    const meta = ci.getDatasetMeta(datasetIndex);
    if (meta.hidden === null) {
      // First time clicking – hide the dataset
      meta.hidden = true;
      this.chartOptions.scales[meta.yAxisID].display = false;
    } else {
      // Subsequent clicks toggle the hidden flag
      meta.hidden = !meta.hidden;
      this.chartOptions.scales[meta.yAxisID].display = !this.chartOptions.scales[meta.yAxisID].display
    }
    // Keep our local visibility array in sync
    this.datasetVisibility[datasetIndex] = !meta.hidden;

    saveDatasetVisibility(this.datasetVisibility);
    ci.update();

    return false; // Prevent default legend click handling
  }

  private get zoomPanFactor(): number {
    return Math.max(1, Math.floor(this.visibleItemCount / 40));
  }

  onMouseWheel(event: WheelEvent) {
    if (!this.isMouseOverChart) return;
    const factor = this.zoomPanFactor;
    if (event.deltaY > 0)
      this.visibleItemCount += factor;
    else
      this.visibleItemCount -= factor;

    this.enforceVisibleItemBounds();
    this.setTimeLimits();
    event.preventDefault();
  }

  onMouseDown(event: MouseEvent) {
    if (!this.isMouseOverChart) return;
    this.mousebuttonpressed = true;
    this.mousestartposition = event.pageX;
    console.log("mousedown");
    return false;
  }

  onMouseUp(event: MouseEvent) {
    if (!this.isMouseOverChart) return;
    this.mousebuttonpressed = false;
    this.mousestartposition = 0;
    console.log("mouseup");
    return true
  }

  private stepcount = 0;

  onMouseMove(event: MouseEvent) {
    if (!this.isMouseOverChart) return;
    const factor = this.zoomPanFactor;
    if (this.mousebuttonpressed && this.stepcount == 1) {
      if (this.mousestartposition > event.pageX) {
        this.itemPosition += factor;
        this.mousestartposition = event.pageX;
      }
      else if (this.mousestartposition < event.pageX) {
        this.itemPosition -= factor;
        this.mousestartposition = event.pageX;
      }
      this.stepcount = 0;

      if (this.itemPosition > 0)
        this.itemPosition = 0;
    } else if (this.mousebuttonpressed)
      this.stepcount++;

    this.setTimeLimits();
    return false;
  }

  private enforceVisibleItemBounds() {
    const total = this.chartData.labels.length;
    if (total === 0) { return; }

    // Ensure we never show more points than available
    if (this.visibleItemCount > total) {
      this.visibleItemCount = total;
    }

    const maxVisible = Math.min(600, total);
    if (this.visibleItemCount > maxVisible) {
      this.visibleItemCount = maxVisible;
    }
    // Minimum visible items is 5 unless fewer points exist
    const minVisible = Math.min(5, total);
    if (this.visibleItemCount < minVisible) {
      this.visibleItemCount = minVisible;
    }

    // Keep itemPosition within valid bounds – allow negative offsets
    const maxOffset = total - this.visibleItemCount;
    if (this.itemPosition < 0) {
      // Offset back cannot exceed the maximum possible offset
      this.itemPosition = Math.max(this.itemPosition, -maxOffset);
    } else {
      // Offset forward limited by maxOffset
      this.itemPosition = Math.min(this.itemPosition, maxOffset);
    }
  }

  private setTimeLimits() {
    // Updated logic to correctly handle limited data points
    const total = this.chartData.labels.length;
    if (total === 0) { return; }

    /* ---- NEW: special case for a single point ---- */
    if (total === 1) {
      const label = this.chartData.labels[0];
      // give the x‑axis a small window around the timestamp
      this.chartOptions.scales.x.min = label - 10000;
      this.chartOptions.scales.x.max = label + 10000;
      (this.chart?.chart as any)?.update();
      return;
    }

    let minIndex = (total - this.visibleItemCount) + this.itemPosition;

    // Clamp minIndex to valid range
    if (minIndex < 0) {
      minIndex = 0;
      this.itemPosition = Math.min(0, this.itemPosition);
    } else if (minIndex > total - 1) {
      minIndex = total - 1;
    }

    // Determine maxIndex based on visibleItemCount
    let maxIndex = minIndex + this.visibleItemCount;

    if (maxIndex > total) {
      maxIndex = total;
    }

    const minLabel = this.chartData.labels[minIndex];
    const maxLabel = this.chartData.labels[maxIndex - 1] ?? this.chartData.labels[total - 1];

    this.chartOptions.scales.x.min = minLabel;
    this.chartOptions.scales.x.max = maxLabel;
    (this.chart?.chart as any)?.update();
    this.checkForPreviousHour();
  }

  /**
   * Load historical data from the server and append it to the chart.
   *
   * @param year  e.g. "2024"
   * @param month e.g. "03"
   * @param day   e.g. "15"
   * @param hour  e.g. "12" (24‑hour format)
   */




  addSocketMessage(msg: SocketMsg): void {
    if (!this.initialized) {
      const { chartData, fullChartData, datasetKeyIndexMap } = initializeDatasets(
        msg,
        this.chartData,
        this.fullChartData,
        this.datasetKeyIndexMap,
        this.chartOptions
      );
      // Assign the returned values back to component fields
      this.chartData = chartData;
      this.fullChartData = fullChartData;
      this.datasetKeyIndexMap = datasetKeyIndexMap;
      this.setTimeRange(this.currentRange);
      this.initialized = true;
    }
    if (!this.chartData.labels.length) {
      this.loadTime(0);
    }

    let timeLabel: number;
    timeLabel = Date.now();

    /* ---- rest of the method stays unchanged ---- */
    if (timeLabel === 0) return;

    const valuesByKey: Record<string, number> = {
      voltage0: Number(msg.voltage0 ?? 0),
      voltage1: Number(msg.voltage1 ?? 0),
      temperature: Number(msg.bme280?.atemperatur ?? 0),
      humidity: Number(msg.bme280?.ahumidity ?? 0),
      co2: Number(msg.ens160aht21?.eco2 ?? 0),
      lightPower: Number(msg.lightvalP ?? 0),
      lightVoltage: Number(msg.lightvalmv ?? 0),
      tempFromEns: Number(msg.ens160aht21?.atemperatur ?? 0),
      humFromEns: Number(msg.ens160aht21?.ahumidity ?? 0),
      pressure: Number(msg.bme280?.apressure)
    };
    const prevTotal = this.chartData.labels.length;
    const wasFullView = (this.visibleItemCount === prevTotal) && this.itemPosition === 0;

    this.chartData.labels.push(timeLabel);
    this.fullChartData.labels.push(timeLabel);
    Object.entries(valuesByKey).forEach(([key, val]) => {
      const idx = this.datasetKeyIndexMap[key];
      if (idx !== undefined) {
        const ds = this.chartData.datasets[idx];
        if (ds && Array.isArray(ds.data)) {
          ds.data.push(val);
        }
        const ds1 = this.fullChartData.datasets[idx];
        if (ds1 && Array.isArray(ds1.data)) {
          ds1.data.push(val);
        }
      }
    });

    this.enforceVisibleItemBounds();
    if (this.chartData.labels.length <= 600) {
      this.itemPosition = 0;
      this.visibleItemCount++;
    } else if (!wasFullView && this.itemPosition < 0) {
      // Move the offset back one more to keep the same earliest point
      this.itemPosition -= 1;
      // Clamp after adjustment
      const maxOffset = this.chartData.labels.length - this.visibleItemCount;
      this.itemPosition = Math.max(this.itemPosition, -maxOffset);
    }
    this.setTimeLimits();
  }

  private loadTime(time: number) {
    const now = new Date();
    const start = new Date(now.getTime() - time * 60 * 60 * 1000);   // 4 h ago

    const year = start.getFullYear().toString();          // e.g. "2024"
    const month = (start.getMonth() + 1).toString().padStart(2, '0'); // "03"
    const day = start.getDate().toString().padStart(2, '0');       // "15"
    const hour = start.getHours().toString().padStart(2, '0');        // "12"

    loadHistoricalData(year, month, day, hour, this.apiService, this.chartData, this.fullChartData);
    this.setTimeRange(this.currentRange);

  }

  private async checkForPreviousHour(): Promise<void> {
    if (this.loadingPreviousHour) return;
    const total = this.chartData.labels.length;
    if (!total) return;

    // We are at the oldest point when minIndex === 0
    const minIndex = (total - this.visibleItemCount) + this.itemPosition;
    if (minIndex !== 0) return; // not at the start

    const firstLabelMs = this.chartData.labels[0];
    const firstDate = new Date(firstLabelMs);
    firstDate.setHours(firstDate.getHours() - 1);   // move to previous hour

    const year = firstDate.getFullYear().toString();
    const month = (firstDate.getMonth() + 1).toString().padStart(2, '0');
    const day = firstDate.getDate().toString().padStart(2, '0');
    const hour = firstDate.getHours().toString().padStart(2, '0');

    // **Remove the guard that prevents repeated loading**
    // this.loadedHours.add(key);

    this.loadingPreviousHour = true;
    try {
      const { chartData: updatedChartData, fullChartData: updatedFullChartData } =
        await loadHistoricalData(
          year,
          month,
          day,
          hour,
          this.apiService,
          this.chartData,
          this.fullChartData
        );
      this.chartData = updatedChartData;
      this.fullChartData = updatedFullChartData;
      /* Keep the view at the oldest point after adding older data */
      this.itemPosition = -(this.chartData.labels.length - this.visibleItemCount);
      this.enforceVisibleItemBounds();
    } finally {
      this.loadingPreviousHour = false;
    }
  }

  setTimeRange(range: '10min' | '30min' | '1h' | '2h' | '4h'): void {
    const sampled = createSampledData(range, this.fullChartData);
    this.chartData.labels = sampled.labels;
    this.chartData.chartData = sampled.datasets;
    this.datasetVisibility = restoreDatasetVisibility(this.datasetVisibility, this.chartData, this.chart, this.chartOptions);
    this.currentRange = range;
    this.visibleItemCount = 600;
    this.itemPosition = 0;
    this.enforceVisibleItemBounds();
    this.setTimeLimits();
  }


  public datasetVisibility: boolean[] = [];

}


