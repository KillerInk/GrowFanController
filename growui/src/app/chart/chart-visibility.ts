

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
    let loadedVisibility = loadDatasetVisibility(datasetVisibility, chartData);

    const chart = charti?.chart as any;
    if (!chart || !chart.data.datasets.length) {
        // Chart not ready yet – try again shortly
        setTimeout(() => restoreDatasetVisibility(loadedVisibility, chartData, charti, chartOptions), 200);
        return datasetVisibility;
    }

    // Ensure visibility array matches dataset count
    const visCount = chart.data.datasets.length;
    if (loadedVisibility.length !== visCount) {
        loadedVisibility = Array.from({ length: visCount }, () => true);
    }

    // Mutate the original array so references stay valid
    datasetVisibility.length = 0;
    for (let i = 0; i < loadedVisibility.length; i++) {
        datasetVisibility[i] = loadedVisibility[i];
    }

    // Apply visibility to each meta and collect which axes should be visible
    const metaHidden: Record<number, boolean> = {};
    const axisShouldDisplay: Record<string, boolean> = {};
    
    chart.data.datasets.forEach((_: any, idx: number) => {
        const meta = chart.getDatasetMeta(idx);
        // loadedVisibility[idx] true means visible, false means hidden
        // meta.hidden = null/false means visible, true means hidden
        if (loadedVisibility[idx]) {
            meta.hidden = null;  // Show the dataset
        } else {
            meta.hidden = true;  // Hide the dataset
        }
        metaHidden[idx] = meta.hidden === true;
        
        // Track which yAxisIDs have visible datasets
        const yAxisId = meta.yAxisID;
        if (!axisShouldDisplay[yAxisId]) {
            axisShouldDisplay[yAxisId] = false;
        }
        // If dataset is visible, mark axis as should display
        if (loadedVisibility[idx]) {
            axisShouldDisplay[yAxisId] = true;
        }
    });

    // Now apply axis display based on whether any visible dataset uses it
    chart.data.datasets.forEach((_: any, idx: number) => {
        const yAxisId = chart.getDatasetMeta(idx).yAxisID;
        const shouldDisplay = axisShouldDisplay[yAxisId] || false;
        
        if (chartOptions.scales && chartOptions.scales[yAxisId]) {
            chartOptions.scales[yAxisId].display = shouldDisplay;
        }
        
        if (chart.options && chart.options.scales && chart.options.scales[yAxisId]) {
            chart.options.scales[yAxisId].display = shouldDisplay;
        }
    });

    chart.update();
    return datasetVisibility;
}