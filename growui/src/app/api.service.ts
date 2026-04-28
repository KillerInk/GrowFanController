// src/app/services/api.service.ts
import { Injectable } from '@angular/core';
import { HttpClient, HttpParams, HttpErrorResponse, HttpEvent } from '@angular/common/http';
import { Observable, throwError } from 'rxjs';
import { catchError, map } from 'rxjs/operators';
import { environment } from '../environments/environment';
import { DeviceState } from './types';

@Injectable({
  providedIn: 'root',
})
export class ApiService {
  private baseUrl = `${environment.apiBaseUrl}:${environment.apiPort}`;

  constructor(private http: HttpClient) { }

  /** Helper that builds query string for /cmd */
  private buildCmdParams(params: Record<string, any>): HttpParams {
    let httpParams = new HttpParams();
    Object.entries(params).forEach(([key, value]) => {
      if (value !== undefined && value !== null) {
        httpParams = httpParams.append(key, String(value));
      }
    });
    return httpParams;
  }

  /** Generic GET to /cmd */
  getCmd<T>(params: Record<string, any>): Observable<T> {
    const url = `/cmd`;
    const httpParams = this.buildCmdParams(params);

    // Options with plain‑text response
    const options = { params: httpParams, responseType: 'text' } as any;

    // TypeScript needs a cast because the generic type T could be anything.
    return (this.http.get<string>(url, options) as unknown as Observable<T>).pipe(
      catchError(this.handleError)
    );
  }

  /** GET /settings */
  getFanControllerSettings(): Observable<DeviceState> {
    const url = `/settings`;
    return this.http.get<DeviceState>(url).pipe(catchError(this.handleError));
  }

  /** GET /data */
  getData(year: string, month: string, day: string, hour: string): Observable<string> {
    const url = `/data`;
    const httpParams = new HttpParams()
      .set('year', year)
      .set('month', month)
      .set('day', day)
      .set('hour', hour);
    return this.http.get<string>(url, { params: httpParams }).pipe(catchError(this.handleError));
  }

  /* ---------- Individual command wrappers ---------- */

  // Example: set speed
  setSpeed(id: number, val: number): Observable<any> {
    if (val < 0 || val > 100) return throwError('speed out of range');

    const params = this.buildCmdParams({ var: 'speed', id, val });

    // Tell HttpClient to treat the response as text
    // Cast options so TypeScript accepts the 'text' value.
    const options = { params, responseType: 'text' } as any;
    return this.http.get<string>(`/cmd`, options).pipe(
      catchError(this.handleError)
    );
  }

  setLight(val: number): Observable<any> {
    if (val < 0 || val > 255) return throwError('light value out of range');

    const params = this.buildCmdParams({ var: 'lightval', val });

    // Tell HttpClient to treat the response as text
    // Cast options so TypeScript accepts the 'text' value.
    const options = { params, responseType: 'text' } as any;
    return this.http.get<string>(`/cmd`, options).pipe(
      catchError(this.handleError)
    );
  }

  // voltage change
  setVoltageLimits(id: number, min: number, max: number): Observable<any> {
    return this.getCmd({ var: 'voltage', id, min, max });
  }

  /* ---------- Error handling ----------
   * You can customize error messages here.
   */
  private handleError(error: HttpErrorResponse) {
    const msg = error.status ? `HTTP ${error.status}: ${error.message}` : 'Network error';
    return throwError(msg);
  }

  setMinMaxSpeed(min: number, max: number): Observable<any> {
    // Validate input ranges if needed (e.g., 0‑100)
    if (min < 0 || min > 100) return throwError('min speed out of range');
    if (max < 0 || max > 100) return throwError('max speed out of range');

    const params = this.buildCmdParams({ var: 'autospeed', min, max });

    // Tell HttpClient to treat the response as text
    const options = { params, responseType: 'text' } as any;
    return this.http.get<string>(`/cmd`, options).pipe(
      catchError(this.handleError)
    );
  }

  setTargetTempHum(temp: number, hum: number, speeddif: number): Observable<any> {
    // Optional validation (e.g., range checks)
    const params = this.buildCmdParams({
      var: 'autovals',
      temp,
      hum,
      speeddif
    });

    // Tell HttpClient to treat the response as text
    const options = { params, responseType: 'text' } as any;
    return this.http.get<string>(`/cmd`, options).pipe(
      catchError(this.handleError)
    );
  }

  setTargetTempHumDiff(temp: number, hum: number): Observable<any> {
    // Optional validation (e.g., range checks)
    const params = this.buildCmdParams({
      var: 'temphumdif',
      temp,
      hum
    });

    // Tell HttpClient to treat the response as text
    const options = { params, responseType: 'text' } as any;
    return this.http.get<string>(`/cmd`, options).pipe(
      catchError(this.handleError)
    );
  }

  setLightAutoControl(val: number): Observable<any> {
    const params = this.buildCmdParams({ var: 'lightautomode', enable: val });

    // Tell HttpClient to treat the response as text
    const options = { params, responseType: 'text' } as any;
    return this.http.get<string>(`/cmd`, options).pipe(
      catchError(this.handleError)
    );
  }


  setNightModeActive(val: number): Observable<any> {
    const params = this.buildCmdParams({ var: 'fannightmodeactive', val });

    // Tell HttpClient to treat the response as text
    const options = { params, responseType: 'text' } as any;
    return this.http.get<string>(`/cmd`, options).pipe(
      catchError(this.handleError)
    );
  }

  setFanAutoControl(val: number): Observable<any> {
    const params = this.buildCmdParams({ var: 'autocontrol', val });

    // Tell HttpClient to treat the response as text
    const options = { params, responseType: 'text' } as any;
    return this.http.get<string>(`/cmd`, options).pipe(
      catchError(this.handleError)
    );
  }

  setLightSchedule(params: Record<string, any>): Observable<any> {
    return this.getCmd(params);   // simply forwards to /cmd
  }

  setReadGovee(enabled: boolean): Observable<any> {
    // `enabled` is sent as 1 or 0 (true → 1, false → 0)
    const params = this.buildCmdParams({
      var: 'readgovee',
      val: enabled ? 1 : 0
    });

    // Tell HttpClient to treat the response as text
    const options = { params, responseType: 'text' } as any;
    return this.http.get<string>(`/cmd`, options).pipe(
      catchError(this.handleError)
    );
  }

  setCloudSimActive(val: number): Observable<any> {
    const params = this.buildCmdParams({
      var: 'cloudsimactive',
      val
    });

    const options = { params, responseType: 'text' } as any;
    return this.http.get<string>(`/cmd`, options).pipe(
      catchError(this.handleError)
    );
  }

  flashSpiffs(file: File): Observable<HttpEvent<string>> {   // <-- return HttpEvent
    const formData = new FormData();
    formData.append('file', file, file.name);

    return this.http.post<string>(
      `/flashspiffs`,
      formData,
      {
        responseType: 'text' as any,
        observe: 'events',          // emit progress events
        reportProgress: true       // enable progress reporting
      }
    );
  }

  flashFirmware(file: File): Observable<HttpEvent<any>> {
    const formData = new FormData();
    formData.append('file', file, file.name);

    return this.http.post(
      `/flashfirmware`,
      formData,
      {
        responseType: 'text' as any,
        observe: 'events',
        reportProgress: true
      }
    );
  }

    /**
    * Alias for flashSpiffs (kept for backward compatibility)
    */
   uploadSpiffs(file: File): Observable<HttpEvent<string>> {
     return this.flashSpiffs(file);
   }

   /**
    * Alias for flashFirmware (kept for backward compatibility)
    */
   uploadFirmware(file: File): Observable<HttpEvent<any>> {
     return this.flashFirmware(file);
   }

    /**
    * Download a CSV file containing historical data.
   *
   * @param year  e.g. "2024"
   * @param month e.g. "03"
   * @param day   e.g. "15"
   * @param hour  e.g. "12" (24‑hour format)
   */
  downloadCsv(year: string, month: string, day: string, hour: string): Observable<string> {
    const url = `/${year}/${month}/${day}/${hour}.csv`;
    return this.http.get(url, { responseType: 'text' }).pipe(
      catchError(this.handleError)
    );
  }

  /* ---------- WiFi configuration ---------- */

  setWifiCredentials(ssid: string, password: string, apFallback?: boolean, timeout?: number): Observable<any> {
    const url = `/wifi-config`;
    const formData = new FormData();
    formData.append('ssid', ssid);
    formData.append('password', password);
    if (apFallback !== undefined) formData.append('ap_fallback', apFallback ? 'on' : 'off');
    if (timeout !== undefined && timeout > 0) formData.append('timeout', String(timeout));
    return this.http.post(url, formData, { responseType: 'text' }).pipe(
      catchError(this.handleError)
    );
  }

  /* ---------- Timezone configuration ---------- */

  /** GET /cmd?action=timezone&get */
  getTimeZone(): Observable<{ offset: number }> {
    return this.getCmd<{ offset: number }>({ var: 'timezone', action: 'get' });
  }

  /** POST /cmd?action=timezone&set&offset=N */
  setTimeZone(offset: number): Observable<{ status: string }> {
    return this.getCmd<{ status: string }>({ var: 'timezone', action: 'set', offset });
  }
}
