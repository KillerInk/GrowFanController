

export function saveDatasetVisibility(datasetVisibility: boolean[]) {
    localStorage.setItem('datasetVisibility', JSON.stringify(datasetVisibility));
}

export function loadDatasetVisibility(datasetVisibility: boolean[], chartData: any) {
    const saved = localStorage.getItem('datasetVisibility');
    if (saved) {
        try {
            const arr = JSON.parse(saved);
            // Ensure that the array length matches the number of datasets
            return Array.from({ length: chartData.datasets.length }, (_, i) => {
                if (Array.isArray(arr) && typeof arr[i] === 'boolean') {
                    return arr[i];
                }
                return true;   // default
            });
        } catch { }
    }
    // Default visibility
    return Array.from({ length: chartData.datasets.length }, () => true);
}

export function restoreDatasetVisibility(datasetVisibility: boolean[], chartData: any, charti:any, chartOptions:any) {
    datasetVisibility = loadDatasetVisibility(datasetVisibility,chartData);

    const chart = charti?.chart as any;
    if (!chart || !chart.data.datasets.length) {
        // Chart not ready yet – try again shortly
        setTimeout(() => datasetVisibility = restoreDatasetVisibility(datasetVisibility,chartData,charti,chartOptions), 200);
        return datasetVisibility;
    }

    // Ensure visibility array matches dataset count
    const visCount = chart.data.datasets.length;
    if (datasetVisibility.length !== visCount) {
        datasetVisibility = Array.from({ length: visCount }, () => true);
    }

    // Apply visibility to each meta
    chart.data.datasets.forEach((_: any, idx: number) => {
        const meta = chart.getDatasetMeta(idx);
        // meta.hidden === null means visible; we want it hidden when !visible
        meta.hidden = !datasetVisibility[idx];
        chartOptions.scales[meta.yAxisID].display = datasetVisibility[idx];
    });

    chart.update();
    return datasetVisibility;
}