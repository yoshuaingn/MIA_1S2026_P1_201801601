import { Component } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { HttpClient } from '@angular/common/http';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [CommonModule, FormsModule],
  templateUrl: './app.html',
  styleUrl: './app.css'
})
export class AppComponent {
  entrada: string = '';
  salida: string = '# Acá se verán todos los mensajes de la ejecución';
  nombreArchivo: string = 'No se ha seleccionado archivo'; // Error corregido

  constructor(private http: HttpClient) {}

  // Error corregido: Cambié click="ejecutar()" por click="onEjecutar()" en el HTML
  // O puedes simplemente renombrar la función aquí:
  onEjecutar() {
    const url = 'http://localhost:8080/execute';
    this.http.post(url, { comando: this.entrada }).subscribe({
      next: (res: any) => this.salida += `\n${res.mensaje}`,
      error: () => this.salida += `\n[ERROR]: Sin conexión con el servidor C++`
    });
  }

  // Error corregido: Agregando la función que falta para el botón de archivo
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

  limpiar() {
    this.entrada = '';
    this.salida = '';
    this.nombreArchivo = 'No se ha seleccionado archivo';
  }
}