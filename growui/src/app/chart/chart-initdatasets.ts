import { SocketMsg } from "../types";
import { colors, yAxisIds } from "./chart-config";


export function initializeDatasets(
    msg: SocketMsg,
    chartData: any,
    fullChartData: any,
    datasetKeyIndexMap: Record<string, number>,
    chartOptions: any
): {chartData: any; fullChartData: any; datasetKeyIndexMap: Record<string, number>} {
    const availableFields = {
      voltage0: msg.voltage0,
      voltage1: msg.voltage1,
      temperature: msg.bme280?.temperatur,
      humidity: msg.bme280?.humidity,
      vpdFromBme: msg.bme280?.vpd,
      co2: msg.ens160aht21?.eco2,
      lightPower: msg.lightvalP,
      lightVoltage: msg.lightvalmv,
      // new fields from ens160aht21
      tempFromEns: msg.ens160aht21?.temperatur,
      humFromEns: msg.ens160aht21?.humidity,
      pressure: msg.bme280?.pressure,
      vpdFromEns: msg.ens160aht21?.vpd,
    };

    const validFields = Object.fromEntries(
      Object.entries(availableFields).filter(([_, v]) => v !== undefined && v !== null)
    );

    if (Object.keys(validFields).length === 0) {
      return { chartData, fullChartData, datasetKeyIndexMap }; // nothing to initialize
    }

    chartData = { labels: [], datasets: [] };

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
        case 'pressure': label = 'Pressure (hpa)'; break;
        case 'vpdFromEns': label = 'VPD Ens'; break;
        case 'vpdFromBme': label = 'VPD Bme'; break;
      }
      const datasetpush = {
        ...commonOpts,
        label,
        display: true,
        backgroundColor: colors[key] + ',0.2',
        borderColor: colors[key],
        yAxisID: yAxisIds[key],
      };
      fullChartData.datasets.push(datasetpush);
      chartData.datasets.push(datasetpush);

      datasetKeyIndexMap[key] = chartData.datasets.length - 1;

      if (!chartOptions.scales[yAxisIds[key]]) {
        const position = index % 2 === 0 ? 'left' : 'right';
        chartOptions.scales[yAxisIds[key]] = {
          position,
          title: { display: false },
          ticks: {
            color: colors[key],
            callback: (value: number) =>
              Number.isInteger(value) ? value.toString() : value.toFixed(2)
          },
          grid: { drawOnChartArea: false },
          display: true,
        }
      }
    });

    // Return the updated data so the caller can assign them
    return { chartData, fullChartData, datasetKeyIndexMap };
}