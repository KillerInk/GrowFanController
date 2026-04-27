// src/app/app.ts
import { Component, HostListener, signal, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { RouterModule, Router } from '@angular/router';
import { DashboardService } from './services/dashboard.service';
import { ApiService } from './api.service';
import { SidebarComponent, NavPage } from './shared/sidebar/sidebar.component';
import { TopbarComponent } from './shared/topbar/topbar.component';

@Component({
  selector: 'app-root',
  imports: [CommonModule, RouterModule, SidebarComponent, TopbarComponent],
  providers: [DashboardService, ApiService],
  templateUrl: './app.html',
  styleUrl: './app.scss',
})
export class App {
  private readonly dashboard = inject(DashboardService);
  private readonly router = inject(Router);

  readonly ds = signal(this.dashboard);
  readonly sidebarOpen = signal(false);
  readonly sidebarMinimized = signal(false);
  readonly isMobile = signal(false);
  readonly activePage = signal<NavPage>('dashboard');

  constructor() {
    this.checkMobile();
  }

  @HostListener('window:resize')
  onResize(): void {
    this.checkMobile();
  }

  private checkMobile(): void {
    this.isMobile.set(window.innerWidth <= 768);
    if (!this.isMobile()) {
      this.sidebarOpen.set(false);
    }
  }

  toggleSidebar(): void {
    this.sidebarOpen.update(v => !v);
  }

  closeSidebar(): void {
    this.sidebarOpen.set(false);
  }

  onNavChange(page: NavPage): void {
    this.activePage.set(page);
    this.router.navigate([page]);
  }
}
