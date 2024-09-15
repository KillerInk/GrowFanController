const timeVals = [];
const tempVals = [];
const humVals = [];
const fanspeedVals = [];
const lightmvVals = [];
const co2Vals = [];
const vpdairVals = [];

var visibletimeVals = [];
var visibletempVals = [];
var visiblehumVals = [];
var visiblefanspeedVals = [];
var visiblelightmvVals = [];
var visibleco2Vals = [];
var visiblevpdairVals = [];

var position = 0;
var maxvisibleitems = 50;
var now;

function getChartDataForDay(date, newdata) {
    return new Promise((resolve, reject) => {
        let host = document.location.origin;
        let year = date.getUTCFullYear();
        let month = date.getMonth() + 1;
        if (month < 10)
            month = 0 + "" + month;
        let day = date.getDate();
        let hour = date.getHours();
        const da = `${year}/${month}/${day}/${hour}.csv`;
        const query = `${host}/${da}`;
        fetch(query)
            .then((response) => response.text(), reject)
            .then(data => {
                const lines = data.split("\r\n");
                for (let i = lines.length - 1; i >= 0; i--) {
                    const vals = lines[i].split(", ");
                    addChartItems(vals[0], vals[1], vals[2], vals[3], vals[5], vals[4], vals[6], newdata);
                }
                mychart.config.options.scales.x.max = timeVals.length;
                mychart.update();
                resolve('resolved');
            }, reject);
    });
}

function updateChartPosition() {
    if (timeVals.length - mychart.config.options.scales.x.max < 10) {
        let dif = mychart.config.options.scales.x.max - mychart.config.options.scales.x.min;
        mychart.config.options.scales.x.max = timeVals.length;
        mychart.config.options.scales.x.min = timeVals.length - dif;
    }
}

async function getChartDataForToday() {
    now = new Date();
    getChartDataForDay(now, false).then(now => {
        now.setHours(now.getHours() - 1);
        getChartDataForDay(now, false).then(()=>{
            setTimeRangeToShow(5*60);//5min
            updateChartPosition();
        });
    });
}

async function getNextChartData() {
    now.setHours(now.getHours() - 1);
    mychart.config.options.scales.x.min = 20;
    getChartDataForDay(now, false);
}

function addChartItems(time, temp, hum, fanspeed, light, co2, vdp, push) {
    if (push) {
        timeVals.push(time);
        tempVals.push(temp);
        humVals.push(hum);
        fanspeedVals.push(fanspeed);
        lightmvVals.push(light);
        co2Vals.push(co2);
        vpdairVals.push(vdp);
    }
    else {
        timeVals.unshift(time);
        tempVals.unshift(temp);
        humVals.unshift(hum);
        fanspeedVals.unshift(fanspeed);
        lightmvVals.unshift(light);
        co2Vals.unshift(co2);
        vpdairVals.unshift(vdp);
    }
}

var mychart = new Chart("myChart", {
    type: "line",
    data: {
        labels: timeVals,
        datasets: [{
            label: 'Temperatur',
            fill: false,
            borderColor: "red",
            data: tempVals,
            yAxisID: 'y1',
            pointRadius: 0,
            borderWidth: 1,
        },
        {
            label: 'Humidity',
            fill: false,
            borderColor: "blue",
            data: humVals,
            yAxisID: 'y2',
            pointRadius: 0,
            borderWidth: 1,
        },
        {
            label: 'Fanspeed',
            fill: false,
            borderColor: "black",
            data: fanspeedVals,
            yAxisID: 'y3',
            pointRadius: 0,
            borderWidth: 1,
        },
        {
            label: 'Light',
            fill: false,
            borderColor: 'orange',
            data: lightmvVals,
            yAxisID: 'y4',
            pointRadius: 0,
            borderWidth: 1,
        },
        {
            label: 'Co2',
            fill: false,
            borderColor: 'green',
            data: co2Vals,
            yAxisID: 'y5',
            pointRadius: 0,
            borderWidth: 1,
        },
        {
            label: 'Vpd Air',
            fill: false,
            borderColor: 'cyan',
            data: vpdairVals,
            yAxisID: 'y6',
            pointRadius: 0,
            borderWidth: 1,
        }
        ]
    },
    options: {
        spanGaps: true,
        animation: false,
        responsive: false,
        interaction: {
            mode: 'nearest',
            axis: 'x',
            intersect: false
        },
        scales: {
            x: {
                // The axis for this scale is determined from the first letter of the id as `'x'`
                // It is recommended to specify `position` and / or `axis` explicitly.
                //min: 100,
                //max:500,
            },
            y1: {
                type: 'linear',
                display: true,
                position: 'right',
                ticks: { color: "red" },
                //min: 10,
                //max: 35,
            },
            y2: {
                type: 'linear',
                display: true,
                position: 'right',
                ticks: { color: "blue" },

                //min: 20,
                //max: 80,
                // grid line settings
                grid: {
                    drawOnChartArea: false, // only want the grid lines for one axis to show up
                },
            },
            y3: {
                type: 'linear',
                display: true,
                position: 'right',
                ticks: { color: "black" },
                //min: 0,
                //max: 100,
                // grid line settings
                grid: {
                    drawOnChartArea: false, // only want the grid lines for one axis to show up
                },
            },
            y4: {
                type: 'linear',
                display: true,
                position: 'right',
                ticks: { color: "orange" },
                //min: 0,
                //max: 10000,
                // grid line settings
                grid: {
                    drawOnChartArea: false, // only want the grid lines for one axis to show up
                },
            },
            y5: {
                type: 'linear',
                display: true,
                position: 'right',
                ticks: { color: "green" },
                // grid line settings
                //min: 400,
                //max: 65000,
                grid: {
                    drawOnChartArea: false, // only want the grid lines for one axis to show up
                },
            },
            y6: {
                type: 'linear',
                display: true,
                position: 'right',
                ticks: { color: "cyan" },
                //min: 0,
                //max: 3,
                // grid line settings
                grid: {
                    drawOnChartArea: false, // only want the grid lines for one axis to show up
                },
            },
        },
        plugins: {
            zoom: {
                pan: {
                    enabled: true,
                    mode: 'x',
                },
                zoom: {
                    wheel: {
                        enabled: true,
                    },
                    pinch: {
                        enabled: true
                    },
                    mode: 'x',
                }
            }
        }

    }
});

function setTimeRangeToShow(timerange) {
    mychart.config.options.scales.x.min = mychart.config.options.scales.x.max - timerange;
    mychart.update();
};

var button1min = document.getElementById("1min");

button1min.onclick = function () {
    let r = 1 * 60;
    setTimeRangeToShow(r);
};

var button5min = document.getElementById("5min");

button5min.onclick = function () {
    let r = 5 * 60;
    setTimeRangeToShow(r);
};

var button15min = document.getElementById("15min");

button15min.onclick = function () {
    let r = 15 * 60;
    setTimeRangeToShow(r);
};

var button30min = document.getElementById("30min");

button30min.onclick = function () {
    let r = 30 * 60;
    setTimeRangeToShow(r);
};

var button1h = document.getElementById("1h");

button1h.onclick = function () {
    let r = 1 * 60 * 60;
    setTimeRangeToShow(r);
};

var button4h = document.getElementById("4h");

button4h.onclick = function () {
    let r = 4 * 60 * 60;
    setTimeRangeToShow(r);
};

var zoomVals = document.getElementById("zoomdata");

zoomVals.onclick = function () {
    let val = zoomVals.checked;
    if (val == false) {
        mychart.config.options.scales.y1.min = 10;
        mychart.config.options.scales.y1.max = 35;
        mychart.config.options.scales.y2.min = 20;
        mychart.config.options.scales.y2.max = 80;
        mychart.config.options.scales.y3.min = 0;
        mychart.config.options.scales.y3.max = 100;
        mychart.config.options.scales.y4.min = 0;
        mychart.config.options.scales.y4.max = 10000;
        mychart.config.options.scales.y5.min = 400;
        mychart.config.options.scales.y5.max = 2000;
        mychart.config.options.scales.y6.min = 0;
        mychart.config.options.scales.y6.max = 3;
    }
    else {
        delete mychart.config.options.scales.y1.min;
        delete mychart.config.options.scales.y1.max;
        delete mychart.config.options.scales.y2.min;
        delete mychart.config.options.scales.y2.max;
        delete mychart.config.options.scales.y3.min;
        delete mychart.config.options.scales.y3.max;
        delete mychart.config.options.scales.y4.min;
        delete mychart.config.options.scales.y4.max;
        delete mychart.config.options.scales.y5.min;
        delete mychart.config.options.scales.y5.max;
        delete mychart.config.options.scales.y6.min;
        delete mychart.config.options.scales.y6.max;
    }
}