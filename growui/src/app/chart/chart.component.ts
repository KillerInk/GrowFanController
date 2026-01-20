/* growui/src/app/chart/chart.component.ts */
import { Component, Input, ViewChild } from '@angular/core';
import { UIChart } from 'primeng/chart';
import { SocketMsg } from '../types';
import 'chartjs-adapter-date-fns';
import { CommonModule } from '@angular/common';
import { Chart } from 'chart.js';
import { LegendItem } from 'chart.js';

@Component({
  selector: 'app-chart',
  imports: [UIChart, CommonModule],
  styleUrl: "./chart.component.scss",
  templateUrl: './chart.component.html',
})
export class ChartComponent {
  @ViewChild('chart') chart?: UIChart;

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
      x: { display: true, title: { text: 'Time' }, type: 'time', time: { unit: 'second', tooltipFormat: 'HH:mm:ss', displayFormats: { second: 'HH:mm:ss', minute: 'HH:mm', hour: 'HH:mm' } } },
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

  /** Public method called by parent component with raw socket data */
  addSocketMessage(msg: SocketMsg): void {
    if (!this.initialized) {
      this.initializeDatasets(msg);
      this.initialized = true;
    }

    const timeLabel = Date.now();
    const values = [
      msg.voltage0 ?? 0,
      msg.voltage1 ?? 0,
      msg.bme280?.temperatur ?? 0,
      msg.bme280?.humidity ?? 0,
      msg.ens160aht21?.eco2 ?? 0,
      msg.lightvalP ?? 0,
      msg.lightvalmv ?? 0,
      // new fields from ens160aht21
      msg.ens160aht21?.temperatur ?? 0,
      msg.ens160aht21?.humidity ?? 0
    ];

    this.chartData.labels.push(timeLabel);
    values.forEach((v, idx) => {
      const ds = this.chartData.datasets[idx];
      if (ds && Array.isArray(ds.data)) {
        ds.data.push(v);
      }
    });

    // Keep only the last 50 points
    if (this.chartData.labels.length > 50) {
      this.chartData.labels.shift();
      this.chartData.datasets.forEach((ds: any) => ds?.data?.shift());
    }

    this.chart?.chart.update();
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



      const commonOpts = { type: 'line', data: [], tension: 0.3 };
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
}


