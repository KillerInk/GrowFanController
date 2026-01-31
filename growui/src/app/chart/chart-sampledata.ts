export function createSampledData(range: '10min' | '30min' | '1h' | '2h' | '4h', fullChartData: any): { labels: number[]; datasets: any[] } {
    const intervalMap = { '10min': 1, '30min': 3, '1h': 6, '2h': 12, '4h': 24 };
    const step = intervalMap[range] ?? 1;

    // Use the full data set for sampling
    let startIndex = 0;
    let endIndex = fullChartData.labels.length;

    const newLabels: number[] = [];
    const newDatasets = fullChartData.datasets.map((ds: any) => ({
        ...ds,
        data: []
    }));

    for (let i = startIndex; i < endIndex; i += step) {
        newLabels.push(fullChartData.labels[i]);

        fullChartData.datasets.forEach((ds: any, idx: number) => {
            if (Array.isArray(ds.data)) {
                newDatasets[idx].data.push(ds.data[i]);
            }
        });
    }

    return { labels: newLabels, datasets: newDatasets };
}