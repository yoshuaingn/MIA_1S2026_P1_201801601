#include "DiskManager.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <sstream>
#include <cmath>

namespace fs = std::filesystem;

DiskManager::DiskManager() {}

std::string DiskManager::mkdisk(int size, std::string unit, std::string path, char fit) {
    try {
        if (size <= 0) return "ERROR: Tamaño inválido.";

        fs::path dir = fs::path(path).parent_path();
        if (!fs::exists(dir)) {
            fs::create_directories(dir);
        }

        int bytes = size * 1024 * 1024;

        FILE *file = fopen(path.c_str(), "wb+");
        if (!file) return "ERROR: No se pudo crear el disco.";

      
        char cero = '\0';
        for (int i = 0; i < bytes; i++) {
            fwrite(&cero, 1, 1, file);
        }

     
        MBR mbr;
        mbr.mbr_tamano = bytes;
        mbr.mbr_fecha_creacion = time(nullptr);
        mbr.mbr_dsk_signature = rand();  
        mbr.dsk_fit = fit;              

    
        for (int i = 0; i < 4; i++) {
            mbr.mbr_partitions[i].part_status = '0';
            mbr.mbr_partitions[i].part_start = -1;
            mbr.mbr_partitions[i].part_s = 0;
            strcpy(mbr.mbr_partitions[i].part_name, "");
        }

        fseek(file, 0, SEEK_SET);
        fwrite(&mbr, sizeof(MBR), 1, file);

        fclose(file);

        return "MKDISK: Disco creado correctamente.";

    } catch (...) {
        return "ERROR: Fallo al crear disco.";
    }
}

std::string DiskManager::fdisk(int size, std::string unit, std::string path, std::string name, char type, char fit) {

    int tamano = size * 1024 * 1024;

    FILE *file = fopen(path.c_str(), "rb+");
    if (!file) return "ERROR: No existe el disco.";

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    int index = -1;

    for (int i = 0; i < 4; i++) {
        if (mbr.mbr_partitions[i].part_status == '0') {
            index = i;
            break;
        }
    }

    if (index == -1) {
        fclose(file);
        return "ERROR: No hay espacio para más particiones.";
    }

    int start = sizeof(MBR);

    mbr.mbr_partitions[index].part_status = '1';
    mbr.mbr_partitions[index].part_type = type;
    mbr.mbr_partitions[index].part_fit = fit;
    mbr.mbr_partitions[index].part_start = start;
    mbr.mbr_partitions[index].part_s = tamano;
    strcpy(mbr.mbr_partitions[index].part_name, name.c_str());

    fseek(file, 0, SEEK_SET);
    fwrite(&mbr, sizeof(MBR), 1, file);

    fclose(file);

    return "FDISK: Partición creada correctamente.";
}


std::string DiskManager::mount(std::string path, std::string name) {

    FILE *file = fopen(path.c_str(), "rb");
    if (!file) return "ERROR: No se encontró el disco.";

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);
    fclose(file);

    for (int i = 0; i < 4; i++) {
        if (mbr.mbr_partitions[i].part_status == '1' &&
            strcmp(mbr.mbr_partitions[i].part_name, name.c_str()) == 0) {

            MountNode nodo;
            nodo.path = path;
            nodo.name = name;
            nodo.start = mbr.mbr_partitions[i].part_start;
            nodo.size = mbr.mbr_partitions[i].part_s;

            nodo.id = "vd" + std::to_string(particionesMontadas.size() + 1);

            particionesMontadas.push_back(nodo);

            return "MOUNT: OK → " + nodo.id;
        }
    }

    return "ERROR: Partición no encontrada.";
}

std::string DiskManager::rep_disk(std::string path, std::string output) {
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) return "ERROR: No se pudo abrir disco";

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    int total = mbr.mbr_tamano;

    std::stringstream dot;
    dot << "digraph G {\n";
    dot << "node [shape=plaintext]\n";
    dot << "tabla [label=<\n";
    dot << "<table border='1' cellborder='1'>\n";
    dot << "<tr>\n";

    
    float porcentajeMBR = (float)sizeof(MBR) / total * 100;
    dot << "<td>MBR<br/>" << round(porcentajeMBR) << "%</td>\n";

    int usado = sizeof(MBR);

    for (int i = 0; i < 4; i++) {

        if (mbr.mbr_partitions[i].part_status == '1') {

            int size = mbr.mbr_partitions[i].part_s;
            float porcentaje = (float)size / total * 100;

            dot << "<td>Primaria<br/>"
                << mbr.mbr_partitions[i].part_name
                << "<br/>" << round(porcentaje) << "%</td>\n";

            usado += size;
        }
    }

    
    if (usado < total) {
        float libre = (float)(total - usado) / total * 100;
        dot << "<td>Libre<br/>" << round(libre) << "%</td>\n";
    }

    dot << "</tr>\n</table>\n>];\n}";
    
    fclose(file);

   
    std::string dotPath = "/tmp/disk.dot";
    std::ofstream out(dotPath);
    out << dot.str();
    out.close();

    
    std::string cmd = "dot -Tpng " + dotPath + " -o \"" + output + "\"";
    system(cmd.c_str());

    return "REPORTE DISK GENERADO";
}