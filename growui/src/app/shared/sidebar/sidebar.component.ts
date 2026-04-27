// src/app/shared/sidebar/sidebar.component.ts
import { Component, input, output } from '@angular/core';
import { CommonModule } from '@angular/common';

export type NavPage = 'dashboard' | 'fans' | 'lights' | 'sensors' | 'system';

const navItems: { key: NavPage; icon: string; label: string }[] = [
  { key: 'dashboard', icon: '📊', label: 'Dashboard' },
  { key: 'fans', icon: '🌀', label: 'Fans' },
  { key: 'lights', icon: '💡', label: 'Lights' },
  { key: 'sensors', icon: '🌡️', label: 'Sensors' },
  { key: 'system', icon: '⚙️', label: 'System' },
];

@Component({
  selector: 'app-sidebar',
  imports: [CommonModule],
  templateUrl: './sidebar.component.html',
  styleUrl: './sidebar.component.scss',
})
export class SidebarComponent {
  readonly mobile = input.required<boolean>();
  readonly activeNav = input<NavPage>('dashboard');
  readonly minimized = input(false);
  readonly minimizedChange = output<boolean>();
  readonly navChange = output<NavPage>();

  readonly navItems = navItems;

  protected isMinimized = false;

  selectPage(page: NavPage): void {
    this.navChange.emit(page);
  }

  closeMobile(): void {
    this.navChange.emit('dashboard');
  }

  toggleMinimize(): void {
    this.isMinimized = !this.isMinimized;
    this.minimizedChange.emit(this.isMinimized);
  }
}
