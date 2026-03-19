import { Component, ElementRef, ViewChild } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { HttpClient, HttpClientModule } from '@angular/common/http';
import { ChangeDetectorRef } from '@angular/core';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [CommonModule, FormsModule, HttpClientModule],
  templateUrl: './app.html',
  styleUrl: './app.css'
})
export class AppComponent {

  entrada: string = '';
  salida: string = '';
  historial: string[] = [];
  nombreArchivo: string = 'nuevo_script.mia';
  imagenesReportes: string[] = [];
  cargando: boolean = false;

  @ViewChild('console') consoleRef!: ElementRef;

  constructor(private http: HttpClient, private cd: ChangeDetectorRef) {}

  onEjecutar() {

  if (!this.entrada.trim() || this.cargando) return;

  this.cargando = true;

  this.http.post<any>('http://localhost:8080/execute', {
    comando: this.entrada
  }).subscribe({
    next: (res) => {

      this.salida = res.log || '';
      this.imagenesReportes = res.images || [];
      this.historial.push(this.salida);

      if (res.image && res.image.length > 50) {
        this.imagenesReportes.push(res.image);
      }

      this.cargando = false;

      this.cd.detectChanges(); // 🔥 CLAVE

      setTimeout(() => this.autoScroll(), 50);
    },
    error: () => {
      this.salida = "❌ Error conectando con el servidor";
      this.cargando = false;
    }
  });
}

  autoScroll() {
    if (this.consoleRef) {
      this.consoleRef.nativeElement.scrollTop =
        this.consoleRef.nativeElement.scrollHeight;
    }
  }

  limpiar() {
    this.entrada = '';
    this.salida = '';
    this.historial = [];
    this.imagenesReportes = [];
  }

  copiarSalida() {
    navigator.clipboard.writeText(this.salida);
  }

  onFileSelected(event: any) {
    const file: File = event.target.files[0];
    if (file) {
      this.nombreArchivo = file.name;

      const reader = new FileReader();
      reader.onload = (e) => {
        this.entrada = e.target?.result as string;
      };
      reader.readAsText(file);
    }
  }
}