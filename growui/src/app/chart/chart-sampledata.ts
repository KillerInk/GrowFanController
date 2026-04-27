export function createSampledData(range: '10min' | '30min' | '1h' | '2h' | '4h', fullChartData: any): { labels: number[]; datasets: any[] } {
    const intervalMap = { '10min': 1, '30min': 3, '1h': 6, '2h': 12, '4h': 24 };
    const step = intervalMap[range] ?? 1;

    // Sample from the END of the dataset (latest data)
    const totalPoints = fullChartData.labels.length;
    const endIndex = totalPoints;
    const targetPoints = Math.min(600, totalPoints);
    const startIndex = Math.max(0, endIndex - targetPoints);

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