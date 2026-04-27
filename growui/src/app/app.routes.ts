// src/app/app.routes.ts
import { Routes } from '@angular/router';

export const routes: Routes = [
  {
    path: '',
    redirectTo: 'dashboard',
    pathMatch: 'full',
  },
  {
    path: 'dashboard',
    loadComponent: () =>
      import('./features/dashboard/dashboard.component').then(m => m.DashboardComponent),
  },
  {
    path: 'fans',
    loadComponent: () =>
      import('./features/fans/fans.component').then(m => m.FansComponent),
  },
  {
    path: 'lights',
    loadComponent: () =>
      import('./features/lights/lights.component').then(m => m.LightsComponent),
  },
  {
    path: 'sensors',
    loadComponent: () =>
      import('./features/sensors/sensors.component').then(m => m.SensorsComponent),
  },
  {
    path: 'system',
    loadComponent: () =>
      import('./features/system/system.component').then(m => m.SystemComponent),
  },
  {
    path: '**',
    redirectTo: 'dashboard',
  },
];
