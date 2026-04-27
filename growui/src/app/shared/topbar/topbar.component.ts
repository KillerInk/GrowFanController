// src/app/shared/topbar/topbar.component.ts
import { Component, input, computed } from '@angular/core';
import { CommonModule } from '@angular/common';
import { DashboardService } from '../../services/dashboard.service';

@Component({
  selector: 'app-topbar',
  imports: [CommonModule],
  templateUrl: './topbar.component.html',
  styleUrl: './topbar.component.scss',
})
export class TopbarComponent {
  readonly dashboard = input.required<DashboardService>();
  readonly isMobile = input.required<boolean>();
  readonly toggleSidebar = input.required<() => void>();

  socketdata = computed(() => this.dashboard().socketdata());
  deviceState = computed(() => this.dashboard().deviceState());
}
