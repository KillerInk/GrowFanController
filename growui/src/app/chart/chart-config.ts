export const yAxisIds: Record<string, string> = {
    voltage0: 'yVoltage0',
    voltage1: 'yVoltage1',
    temperature: 'yTemperature',
    humidity: 'yHumidity',
    co2: 'yCO2',
    lightPower: 'yLightPower',
    lightVoltage: 'yLightVoltage',
    tempFromEns: 'yTempFromEns',
    humFromEns: 'yHumFromEns',
    pressure: 'yPressure'
};

export const colors: Record<string, string> = {
    voltage0: 'rgba(255,99,132,1)',   // red – fan voltage
    voltage1: 'rgba(54,162,235,1)',   // blue – second fan voltage
    temperature: 'rgba(75,192,192,1)',    // teal – ambient temp
    humidity: 'rgba(153,102,255,1)',  // purple – humidity
    co2: 'rgba(255,159,64,1)',   // orange – CO₂ ppm
    lightPower: 'rgba(199,199,199,1)',  // gray – light power %
    lightVoltage: 'rgba(83,102,255,1)',  // indigo – light voltage mV
    tempFromEns: 'rgba(50,205,50,1)',     // green – ENS temperature
    humFromEns: 'rgba(218,165,32,1)',    // goldenrod – ENS humidity
    pressure: 'rgb(165, 32, 218)'
};
//time,tempB,humB,avgTempB,avgHumB,presB,avgPresB,vpdAirB,tempE,humE,avgTempE,avgHumE,eco2,aqi,tvoc,vpdAirE,volt0,volt1,lightP,lightMv
export const csv_yAxisIds: Record<string, string> = {
    volt0: 'yVoltage0',
    volt1: 'yVoltage1',
    tempB: 'yTemperature',
    humB: 'yHumidity',
    eco2: 'yCO2',
    lightP: 'yLightPower',
    lightMv: 'yLightVoltage',
    tempE: 'yTempFromEns',
    humE: 'yHumFromEns',
    presB: 'yPressure'
};


export const chartOptionsBase = {
    animation: false,
    responsive: true,
    maintainAspectRatio: false,
    interaction: { intersect: false, mode: 'index' },

    

    scales: {
        x: {
            display: true,
            title: { text: 'Time' },
            type: 'time',
            time: {
                unit: 'second',
                tooltipFormat: 'HH:mm:ss',
                displayFormats: { second: 'HH:mm:ss', minute: 'HH:mm', hour: 'HH:mm' },
                stepSize: 1000,
            },
            ticks: { maxTicksLimit: 100 }
        },
    }
};

