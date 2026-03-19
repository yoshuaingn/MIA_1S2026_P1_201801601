#include <iostream>
#include <ctime>
#include <fstream>
#include <cstring>

typedef struct {
    char part_status;     
    char part_type;     
    char part_fit;        
    int part_start;      
    int part_s;           
    char part_name[16];   
} Partition;

typedef struct {
    int mbr_tamano;       
    time_t mbr_fecha_creacion;
    int mbr_dsk_signature; 
    char dsk_fit;         
    Partition mbr_partitions[4]; 
} MBR;

void crearDisco(int size, std::string unidad, std::string path, char fit) {
    
    int tamano_bytes = size;
    if (unidad == "k") tamano_bytes *= 1024;
    else tamano_bytes *= (1024 * 1024);

    FILE *file = fopen(path.c_str(), "wb");
    if (!file) {
        std::cout << "Error al crear el disco en la ruta: " << path << std::endl;
        return;
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int pasos = tamano_bytes / 1024;
    for (int i = 0; i < pasos; i++) {
        fwrite(buffer, 1024, 1, file);
    }

    MBR disco;
    disco.mbr_tamano = tamano_bytes;
    disco.mbr_fecha_creacion = time(nullptr);
    disco.mbr_dsk_signature = rand() % 1000;
    disco.dsk_fit = fit;
    for(int i=0; i<4; i++) disco.mbr_partitions[i].part_status = '0';

    fseek(file, 0, SEEK_SET);
    fwrite(&disco, sizeof(MBR), 1, file);
    fclose(file);
    std::cout << "Disco creado con exito." << std::endl;
}

void crearParticionPrimaria(std::string path, std::string name, int size, char fit) {
    FILE *file = fopen(path.c_str(), "rb+"); 
    if (!file) return;

    MBR disco;
    fread(&disco, sizeof(MBR), 1, file); 

    
    int indice = -1;
    for (int i = 0; i < 4; i++) {
        if (disco.mbr_partitions[i].part_status == '0') {
            indice = i;
            break;
        }
    }

    if (indice != -1) {
        
        int start = sizeof(MBR); 
        if (indice > 0) {
            start = disco.mbr_partitions[indice-1].part_start + disco.mbr_partitions[indice-1].part_s;
        }

        disco.mbr_partitions[indice].part_status = '1';
        disco.mbr_partitions[indice].part_type = 'P';
        disco.mbr_partitions[indice].part_fit = fit;
        disco.mbr_partitions[indice].part_start = start;
        disco.mbr_partitions[indice].part_s = size;
        strcpy(disco.mbr_partitions[indice].part_name, name.c_str());

        // Volver a escribir el MBR actualizado
        fseek(file, 0, SEEK_SET);
        fwrite(&disco, sizeof(MBR), 1, file);
        std::cout << "Particion creada: " << name << std::endl;
    }
    fclose(file);
}
