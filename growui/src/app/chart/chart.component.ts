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
import { firstValueFrom } from 'rxjs';

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
    animation: false,
    responsive: true,
    maintainAspectRatio: false,
    interaction: { intersect: false, mode: 'index' },

    plugins: {
      legend: { display: true, onClick: (e: any, legendItem: any, legend: any) => this.onLegendClick(e, legendItem, legend) },
    },

    scales: {
      x: {
        display: true,
        title: { text: 'Time' },
        type: 'time',
        time: {
          unit: 'second',
          tooltipFormat: 'HH:mm:ss',
          displayFormats: { second: 'HH:mm:ss', minute: 'HH:mm', hour: 'HH:mm' },
          /* Tell Chart.js that the expected step is 1 second (1000 ms) */
          stepSize: 1000,
        },
        ticks: { maxTicksLimit: 100 }
      },
    }
  };

  /** Internal chart data – initialized on first message */
  chartData: any = { labels: [], datasets: [] };
  private initialized = false;

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

    const maxVisible = Math.min(720, total);
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

  private loadHistoricalData(year: string, month: string, day: string, hour: string): Promise<void> {
    const maxGapMs = 10000; // increase allowed gap to 10 seconds (or Infinity)
    //time,tempE,humE,avgTempE,avgHumE,eco2,aqi,tvoc,vpdAirE,volt0,volt1,lightP,lightMv
    const yAxisIds: Record<string, string> = {
      volt0: 'yVoltage0',
      volt1: 'yVoltage1',
      tempB: 'yTemperature',
      humB: 'yHumidity',
      eco2: 'yCO2',
      lightP: 'yLightPower',
      lightMv: 'yLightVoltage',
      tempE: 'yTempFromEns',
      humE: 'yHumFromEns'
    };
    return new Promise<void>(async (resolve) => {
      await firstValueFrom(
        this.apiService.downloadCsv(year, month, day, hour).pipe(
          tap((csv: string) => {  // <-- type the csv
            const lines = csv.split('\r\n');
            const header = lines.shift()?.split(',') || [];

            /* Build a mapping from CSV column index → dataset index */
            const colIdxToDsIdx: Record<number, number> = {};
            header.forEach((colName, idx) => {
              const targetYAxisId = yAxisIds[colName];
              if (!targetYAxisId) return; // skip columns that don't map
              const dsIdx = this.chartData.datasets.findIndex(
                (ds: any) => ds.yAxisID === targetYAxisId   // explicit type
              );
              if (dsIdx !== -1) colIdxToDsIdx[idx] = dsIdx;
            });

            let prevTime: number | null = null;

            for (const line of lines) {
              if (!line.trim()) continue;
              const parts = line.split(',');
              const timeLabel = Number(parts[0]) * 1000; // raw timestamp

              // Skip any zero timestamps
              if (timeLabel === 0) continue;

              // Filter out large gaps between consecutive timestamps
              if (prevTime !== null && Math.abs(timeLabel - prevTime) > maxGapMs) {
                prevTime = timeLabel;
                continue;
              }

              const values: number[] = parts.slice(1).map((v: string) => Number(v));

              this.chartData.labels.push(timeLabel);
              values.forEach((v: number, idx: number) => {
                const dsIdx = colIdxToDsIdx[idx + 1];
                if (dsIdx === undefined) return; // skip unmapped columns
                const ds = this.chartData.datasets[dsIdx];
                if (ds && Array.isArray(ds.data)) {
                  ds.data.push(v);
                }
              });

              prevTime = timeLabel;
            }

            /* ---- NEW: sort data by timestamp ---- */
            const sortedIndices: number[] = this.chartData.labels
              .map((label: number, idx: number) => ({ label, idx }))
              .sort(
                (a: { label: number; idx: number }, b: { label: number; idx: number }) =>
                  a.label - b.label
              )
              .map((item: { label: number; idx: number }) => item.idx);

            /* Reorder labels and dataset data accordingly */
            this.chartData.labels = sortedIndices.map(i => this.chartData.labels[i]);

            for (const ds of this.chartData.datasets) {
              if (Array.isArray(ds.data)) {
                ds.data = sortedIndices.map(i => ds.data[i]);
              }
            }

            /* ---- NEW: update visibleItemCount *before* we adjust limits ---- */
            this.enforceVisibleItemBounds();
            this.visibleItemCount = this.chartData.labels.length;   // ensures history is counted
            this.setTimeLimits();
            this.chart?.chart.update();
          })
        )
      );
      resolve();  // satisfy Promise<void>
    });
  }

  addSocketMessage(msg: SocketMsg): void {
    if (!this.initialized) {
      this.initializeDatasets(msg);
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
      temperature: Number(msg.bme280?.temperatur ?? 0),
      humidity: Number(msg.bme280?.humidity ?? 0),
      co2: Number(msg.ens160aht21?.eco2 ?? 0),
      lightPower: Number(msg.lightvalP ?? 0),
      lightVoltage: Number(msg.lightvalmv ?? 0),
      tempFromEns: Number(msg.ens160aht21?.temperatur ?? 0),
      humFromEns: Number(msg.ens160aht21?.humidity ?? 0)
    };
    const prevTotal = this.chartData.labels.length;
    const wasFullView = (this.visibleItemCount === prevTotal) && this.itemPosition === 0;

    this.chartData.labels.push(timeLabel);
    Object.entries(valuesByKey).forEach(([key, val]) => {
      const idx = this.datasetKeyIndexMap[key];
      if (idx !== undefined) {
        const ds = this.chartData.datasets[idx];
        if (ds && Array.isArray(ds.data)) {
          ds.data.push(val);
        }
      }
    });

    // keep only the last 50 points – optional
    // if (this.chartData.labels.length > 50) { … }

    this.enforceVisibleItemBounds();
    if (!wasFullView && this.itemPosition < 0) {
      // Move the offset back one more to keep the same earliest point
      this.itemPosition -= 1;
      // Clamp after adjustment
      const maxOffset = this.chartData.labels.length - this.visibleItemCount;
      this.itemPosition = Math.max(this.itemPosition, -maxOffset);
    }
    this.setTimeLimits();
    this.chart?.chart.update();
  }

  private loadTime(time: number) {
    const now = new Date();
    const start = new Date(now.getTime() - time * 60 * 60 * 1000);   // 4 h ago

    const year = start.getFullYear().toString();          // e.g. "2024"
    const month = (start.getMonth() + 1).toString().padStart(2, '0'); // "03"
    const day = start.getDate().toString().padStart(2, '0');       // "15"
    const hour = start.getHours().toString().padStart(2, '0');        // "12"

    this.loadHistoricalData(year, month, day, hour);
  }

  /** Build datasets based on available fields in the first message */
  private initializeDatasets(msg: SocketMsg): void {
    const availableFields = {
      voltage0: msg.voltage0,
      voltage1: msg.voltage1,
      temperature: msg.bme280?.temperatur,
      humidity: msg.bme280?.humidity,
      co2: msg.ens160aht21?.eco2,
      lightPower: msg.lightvalP,
      lightVoltage: msg.lightvalmv,
      // new fields from ens160aht21
      tempFromEns: msg.ens160aht21?.temperatur,
      humFromEns: msg.ens160aht21?.humidity
    };

    const validFields = Object.fromEntries(
      Object.entries(availableFields).filter(([_, v]) => v !== undefined && v !== null)
    );

    if (Object.keys(validFields).length === 0) {
      return;
    }

    this.chartData = { labels: [], datasets: [] };
    const yAxisIds: Record<string, string> = {
      voltage0: 'yVoltage0',
      voltage1: 'yVoltage1',
      temperature: 'yTemperature',
      humidity: 'yHumidity',
      co2: 'yCO2',
      lightPower: 'yLightPower',
      lightVoltage: 'yLightVoltage',
      tempFromEns: 'yTempFromEns',
      humFromEns: 'yHumFromEns'
    };

    const colors: Record<string, string> = {
      voltage0: 'rgba(255,99,132,1)',   // red – fan voltage
      voltage1: 'rgba(54,162,235,1)',   // blue – second fan voltage
      temperature: 'rgba(75,192,192,1)',    // teal – ambient temp
      humidity: 'rgba(153,102,255,1)',  // purple – humidity
      co2: 'rgba(255,159,64,1)',   // orange – CO₂ ppm
      lightPower: 'rgba(199,199,199,1)',  // gray – light power %
      lightVoltage: 'rgba(83,102,255,1)',  // indigo – light voltage mV
      tempFromEns: 'rgba(50,205,50,1)',     // green – ENS temperature
      humFromEns: 'rgba(218,165,32,1)'    // goldenrod – ENS humidity
    };

    Object.entries(validFields).forEach(([key, _], index) => {

      const commonOpts = {
        type: 'line', data: [],
        borderWidth: 1,
        pointRadius: 0,
        pointHoverRadius: 0,
        tension: 0
      };
      let label = '';
      switch (key) {
        case 'voltage0': label = 'Fan Voltage'; break;
        case 'voltage1': label = 'Fan2 Voltage'; break;
        case 'temperature': label = 'Temperature (°C)'; break;
        case 'humidity': label = 'Humidity (%)'; break;
        case 'co2': label = 'CO₂ (ppm)'; break;
        case 'lightPower': label = 'Light Power (%)'; break;
        case 'lightVoltage': label = 'Light Voltage (mV)'; break;
        // new labels
        case 'tempFromEns': label = 'ENS Temperature (°C)'; break;
        case 'humFromEns': label = 'ENS Humidity (%)'; break;
      }
      this.chartData.datasets.push({
        ...commonOpts,
        label,
        display: true,
        backgroundColor: colors[key] + ',0.2',
        borderColor: colors[key],
        yAxisID: yAxisIds[key],
      });

      this.datasetKeyIndexMap[key] = this.chartData.datasets.length - 1;

      if (!this.chartOptions.scales[yAxisIds[key]]) {
        const position = index % 2 === 0 ? 'left' : 'right';
        this.chartOptions.scales[yAxisIds[key]] = {
          position,
          title: { display: false },
          ticks: {
            color: colors[key],
          },
          grid: { drawOnChartArea: false },
          display: true,
        }
      }
    });
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

    const key = `${year}-${month}-${day}-${hour}`;
    if (this.loadedHours.has(key)) return;

    this.loadingPreviousHour = true;
    try {
      await this.loadHistoricalData(year, month, day, hour);
      /* NEW: after adding older data keep the view at the oldest point */
      this.itemPosition = -(this.chartData.labels.length - this.visibleItemCount);
      this.enforceVisibleItemBounds();
      this.loadedHours.add(key);
    } finally {
      this.loadingPreviousHour = false;
    }
  }
}


