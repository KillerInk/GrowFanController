// src/app/services/websocket.service.ts
import { Injectable } from '@angular/core';
import { webSocket, WebSocketSubject } from 'rxjs/webSocket';
import { environment } from '../environments/environment';
import { Observable } from 'rxjs';

@Injectable({
  providedIn: 'root',
})
export class WebsocketService {
  private wsUrl = `${environment.apiBaseUrl}:${environment.apiPort}/ws`;
  private socket$: WebSocketSubject<string>;

  constructor() {
    this.socket$ = webSocket(this.wsUrl);
  }

  /** Observable of all incoming messages */
  onMessage(): Observable<string> {
    return this.socket$.asObservable();
  }

  /** Send a text message to the server */
  send(msg: string): void {
    this.socket$.next(msg);
  }
}