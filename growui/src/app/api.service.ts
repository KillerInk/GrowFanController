// src/app/services/api.service.ts
import { Injectable } from '@angular/core';
import { HttpClient, HttpParams, HttpErrorResponse } from '@angular/common/http';
import { Observable, throwError } from 'rxjs';
import { catchError, map } from 'rxjs/operators';
import { environment } from '../environments/environment';
import { DeviceState } from './types';

@Injectable({
  providedIn: 'root',
})
export class ApiService {
  private baseUrl = `${environment.apiBaseUrl}:${environment.apiPort}`;

  constructor(private http: HttpClient) {}

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
    const url = `${this.baseUrl}/cmd`;
    const httpParams = this.buildCmdParams(params);
    return this.http.get<T>(url, { params: httpParams }).pipe(
      catchError(this.handleError)
    );
  }

  /** GET /settings */
  getFanControllerSettings(): Observable<DeviceState> {
    const url = `${this.baseUrl}/settings`;
    return this.http.get<DeviceState>(url).pipe(catchError(this.handleError));
  }

  /** GET /data */
  getData(year: string, month: string, day: string, hour: string): Observable<string> {
    const url = `${this.baseUrl}/data`;
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
    return this.getCmd({ var: 'speed', id, val });
  }

  // voltage change
  setVoltageLimits(id: number, min: number, max: number): Observable<any> {
    return this.getCmd({ var: 'voltage', id, min, max });
  }

  // ... add wrappers for all other commands
  // (autovals, autocontrol, readgovee, etc.)

  /* ---------- Error handling ----------
   * You can customize error messages here.
   */
  private handleError(error: HttpErrorResponse) {
    const msg = error.status ? `HTTP ${error.status}: ${error.message}` : 'Network error';
    return throwError(msg);
  }
}