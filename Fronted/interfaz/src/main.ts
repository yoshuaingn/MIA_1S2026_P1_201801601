// src/main.ts
import { bootstrapApplication } from '@angular/platform-browser';
import { appConfig } from './app/app.config';
import { AppComponent } from './app/app'; // <--- CAMBIA 'App' por 'AppComponent'

bootstrapApplication(AppComponent, appConfig) // <--- CAMBIA 'App' por 'AppComponent'
  .catch((err) => console.error(err));