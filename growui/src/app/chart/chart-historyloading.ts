import { firstValueFrom, tap } from "rxjs";
import { ApiService } from "../api.service";
import { csv_yAxisIds } from "./chart-config";

export function loadHistoricalData(
    year: string,
    month: string,
    day: string,
    hour: string,
    apiService: ApiService,
    chartData: any,      // for display
    fullChartData: any   // for internal storage
): Promise<{ chartData: any; fullChartData: any }> {
    const maxGapMs = 10000; // increase allowed gap to 10 seconds (or Infinity)

    return new Promise<void>(async (resolve) => {
        await firstValueFrom(
            apiService.downloadCsv(year, month, day, hour).pipe(
                tap((csv: string) => {
                    const lines = csv.split('\r\n');
                    const header = lines.shift()?.split(',') || [];

                    /* Build a mapping from CSV column index → dataset index */
                    const colIdxToDsIdx: Record<number, number> = {};
                    header.forEach((colName, idx) => {
                        const targetYAxisId = csv_yAxisIds[colName];
                        if (!targetYAxisId) return; // skip columns that don't map
                        const dsIdx = chartData.datasets.findIndex(
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

                        /* Update both chartData and fullChartData */
                        fullChartData.labels.push(timeLabel);
                        chartData.labels.push(timeLabel);

                        values.forEach((v: number, idx: number) => {
                            const dsIdx = colIdxToDsIdx[idx + 1];
                            if (dsIdx === undefined) return; // skip unmapped columns
                            const dsFull = fullChartData.datasets[dsIdx];
                            const dsDisplay = chartData.datasets[dsIdx];

                            if (dsFull && Array.isArray(dsFull.data)) {
                                dsFull.data.push(v);
                            }
                            if (dsDisplay && Array.isArray(dsDisplay.data)) {
                                dsDisplay.data.push(v);
                            }
                        });

                        prevTime = timeLabel;
                    }

                    /* Sort labels and dataset data consistently */
                    const sortedIndices: number[] = fullChartData.labels
                        .map((label: number, idx: number) => ({ label, idx }))
                        .sort(
                            (a: { label: number; idx: number }, b: { label: number; idx: number }) =>
                                a.label - b.label
                        )
                        .map((item: { label: number; idx: number }) => item.idx);

                    fullChartData.labels = sortedIndices.map(i => fullChartData.labels[i]);
                    chartData.labels = sortedIndices.map(i => chartData.labels[i]);

                    for (const ds of fullChartData.datasets) {
                        if (Array.isArray(ds.data)) {
                            ds.data = sortedIndices.map(i => ds.data[i]);
                        }
                    }

                    for (const ds of chartData.datasets) {
                        if (Array.isArray(ds.data)) {
                            ds.data = sortedIndices.map(i => ds.data[i]);
                        }
                    }

                })
            )
        );
        resolve();  // satisfy Promise<void>
    }).then(() => ({
        chartData,
        fullChartData
    }));
}