#include "FileSystemManager.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include <vector>
#include <ctime>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include "Estructuras.h"
#include "DiskManager.h"

FileSystemManager::FileSystemManager() {}

extern std::vector<MountNode> particionesMontadas; 


std::string FileSystemManager::mkfs(std::string id) {

    std::string log = "";

    std::string path = "";
    int start = -1;
    int size = -1;

    for (auto &p : particionesMontadas) {
        if (p.id == id) {
            path = p.path;
            start = p.start;
            size = p.size;
            break;
        }
    }

    if (path == "") {
        return "ERROR: ID no encontrado";
    }

    FILE *file = fopen(path.c_str(), "rb+");
    if (!file) {
        return "ERROR: No se pudo abrir el disco";
    }

    Superbloque sb;

    sb.s_filesystem_type = 2;
    sb.s_inodes_count = 10;
    sb.s_blocks_count = 20;
    sb.s_free_blocks_count = 18;
    sb.s_free_inodes_count = 8;

    sb.s_mtime = time(nullptr);
    sb.s_umtime = time(nullptr);
    sb.s_mnt_count = 1;
    sb.s_magic = 0xEF53;

    sb.s_inode_s = sizeof(Inodo);
    sb.s_block_s = sizeof(BloqueCarpeta);

    sb.s_first_ino = 2;
    sb.s_first_blo = 2;

    sb.s_bm_inode_start = start + sizeof(Superbloque);
    sb.s_bm_block_start = sb.s_bm_inode_start + sb.s_inodes_count;

    sb.s_inode_start = sb.s_bm_block_start + sb.s_blocks_count;
    sb.s_block_start = sb.s_inode_start + (sb.s_inodes_count * sizeof(Inodo));

    
    fseek(file, start, SEEK_SET);
    fwrite(&sb, sizeof(Superbloque), 1, file);

    Inodo root;

    root.i_uid = 1;
    root.i_gid = 1;
    root.i_s = 0;
    root.i_atime = time(nullptr);
    root.i_ctime = time(nullptr);
    root.i_mtime = time(nullptr);
    root.i_type = '0';
    root.i_perm = 664;

    for (int i = 0; i < 15; i++) root.i_block[i] = -1;
    root.i_block[0] = 0;

    Inodo users;

    users.i_uid = 1;
    users.i_gid = 1;
    users.i_s = 27;
    users.i_atime = time(nullptr);
    users.i_ctime = time(nullptr);
    users.i_mtime = time(nullptr);
    users.i_type = '1';
    users.i_perm = 664;

    for (int i = 0; i < 15; i++) users.i_block[i] = -1;
    users.i_block[0] = 1;


    BloqueCarpeta carpeta;

    strcpy(carpeta.b_content[0].b_name, ".");
    carpeta.b_content[0].b_inodo = 0;

    strcpy(carpeta.b_content[1].b_name, "..");
    carpeta.b_content[1].b_inodo = 0;

    strcpy(carpeta.b_content[2].b_name, "users.txt");
    carpeta.b_content[2].b_inodo = 1;

    carpeta.b_content[3].b_inodo = -1;


    BloqueArchivo archivo;

    memset(archivo.b_content, 0, sizeof(archivo.b_content));
    strcpy(archivo.b_content, "1,G,root\n1,U,root,root,123\n");


    char bm_inode[10] = {0};
    char bm_block[20] = {0};

    bm_inode[0] = '1';
    bm_inode[1] = '1';

    bm_block[0] = '1';
    bm_block[1] = '1';

    fseek(file, sb.s_bm_inode_start, SEEK_SET);
    fwrite(bm_inode, sizeof(bm_inode), 1, file);

    fseek(file, sb.s_bm_block_start, SEEK_SET);
    fwrite(bm_block, sizeof(bm_block), 1, file);

    fseek(file, sb.s_inode_start, SEEK_SET);
    fwrite(&root, sizeof(Inodo), 1, file);
    fwrite(&users, sizeof(Inodo), 1, file);

   
    fseek(file, sb.s_block_start, SEEK_SET);
    fwrite(&carpeta, sizeof(BloqueCarpeta), 1, file);
    fwrite(&archivo, sizeof(BloqueArchivo), 1, file);

    fclose(file);

    return "MKFS: Sistema de archivos creado correctamente";
}

std::string FileSystemManager::mkdir(std::string path, bool p) {
    if (particionesMontadas.empty()) return "ERROR: No hay particiones montadas.";
    MountNode* nodo = &particionesMontadas[0]; 

    FILE* file = fopen(nodo->path.c_str(), "rb+");
    if(!file) return "ERROR: No se pudo abrir el disco.";

    Superbloque sb;
    fseek(file, nodo->start, SEEK_SET);
    fread(&sb, sizeof(Superbloque), 1, file);

    std::vector<std::string> carpetas;
    std::stringstream ss(path);
    std::string item;
    while (std::getline(ss, item, '/')) if (!item.empty()) carpetas.push_back(item);

    int inodo_padre = 0;
    for (size_t i = 0; i < carpetas.size(); i++) {
        int existente = buscarEnCarpeta(inodo_padre, carpetas[i], file, sb);
        if (existente == -1) {
            if (p || i == carpetas.size() - 1) {
                inodo_padre = crearCarpeta(inodo_padre, carpetas[i], file, sb);
                if (inodo_padre == -1) { fclose(file); return "ERROR: Sin espacio en disco."; }
            } else {
                fclose(file);
                return "ERROR: No existe la ruta intermedia y no se usó -p.";
            }
        } else {
            inodo_padre = existente;
        }
    }

    fseek(file, nodo->start, SEEK_SET);
    fwrite(&sb, sizeof(Superbloque), 1, file);
    fclose(file);
    return "MKDIR: Carpeta creada exitosamente.";
}

std::string FileSystemManager::mkfile(std::string path, int size, std::string cont) {
    if (particionesMontadas.empty()) return "ERROR: No hay particiones montadas.";
    MountNode* nodo = &particionesMontadas[0];

    FILE* file = fopen(nodo->path.c_str(), "rb+");
    if(!file) return "ERROR: No se pudo abrir el disco.";

    Superbloque sb;
    fseek(file, nodo->start, SEEK_SET);
    fread(&sb, sizeof(Superbloque), 1, file);

    size_t last_slash = path.find_last_of('/');
    std::string folder_path = (last_slash == std::string::npos) ? "/" : path.substr(0, last_slash);
    std::string file_name = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);

    int inodo_carpeta = buscarInodoPorPath(folder_path, file, sb);
    if (inodo_carpeta == -1) { fclose(file); return "ERROR: Ruta no encontrada."; }

    if (cont.empty() && size > 0) {
        for (int i = 0; i < size; i++) cont += std::to_string(i % 10);
    }

    int inodo_idx = encontrarBitLibre(sb.s_bm_inode_start, sb.s_inodes_count, file);
    marcarBit(sb.s_bm_inode_start, inodo_idx, '1', file);

    Inodo nuevoInodo;
    nuevoInodo.i_s = cont.size();
    nuevoInodo.i_type = '1';
    nuevoInodo.i_perm = 664;
    for(int i=0; i<15; i++) nuevoInodo.i_block[i] = -1;

    int bloques_necesarios = ceil((float)cont.size() / 64);
    for (int i = 0; i < bloques_necesarios; i++) {
        int bloque_idx = encontrarBitLibre(sb.s_bm_block_start, sb.s_blocks_count, file);
        marcarBit(sb.s_bm_block_start, bloque_idx, '1', file);
        
        BloqueArchivo ba;
        memset(ba.b_content, 0, 64);
        std::string sub = cont.substr(i * 64, 64);
        strcpy(ba.b_content, sub.c_str());
        
        fseek(file, sb.s_block_start + (bloque_idx * 64), SEEK_SET);
        fwrite(&ba, 64, 1, file);
        if (i < 12) nuevoInodo.i_block[i] = bloque_idx;
    }

    fseek(file, sb.s_inode_start + (inodo_idx * sizeof(Inodo)), SEEK_SET);
    fwrite(&nuevoInodo, sizeof(Inodo), 1, file);
    enlazarEnPadre(inodo_carpeta, inodo_idx, file_name, file, sb);

    fseek(file, nodo->start, SEEK_SET);
    fwrite(&sb, sizeof(Superbloque), 1, file);
    fclose(file);
    return "MKFILE: Archivo creado correctamente.";
}

std::string FileSystemManager::cat(std::vector<std::string> files) {
    return "CAT: Función en desarrollo.";
}


std::string FileSystemManager::rep(std::string name, std::string path, std::string id) {

    MountNode* found = nullptr;

    for (auto &p : particionesMontadas) {
        if (p.id == id) {
            found = &p;
            break;
        }
    }

    if (found == nullptr) {
        return "ERROR: ID de partición no encontrado.";
    }

    FILE *file = fopen(found->path.c_str(), "rb");
    if (!file) return "ERROR: No se pudo abrir disco.";

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);
    fclose(file);

    std::string dot =
        "digraph G {\n"
        "node [shape=plaintext]\n"
        "tbl [label=<\n"
        "<table border='1' cellborder='1'>\n"
        "<tr><td colspan='2'>MBR</td></tr>\n";

    dot += "<tr><td>Size</td><td>" + std::to_string(mbr.mbr_tamano) + "</td></tr>\n";
    dot += "<tr><td>Signature</td><td>" + std::to_string(mbr.mbr_dsk_signature) + "</td></tr>\n";

    for (int i = 0; i < 4; i++) {
        if (mbr.mbr_partitions[i].part_status == '1') {
            dot += "<tr><td>Partición</td><td>";
            dot += mbr.mbr_partitions[i].part_name;
            dot += "</td></tr>\n";
        }
    }

    dot += "</table>>];\n}";

    std::string dotPath = "/tmp/reporte.dot";

    std::ofstream out(dotPath);
    out << dot;
    out.close();

    std::string comando = "dot -Tpng " + dotPath + " -o " + path;
    int result = system(comando.c_str());

    if(result != 0){
        return "ERROR: Graphviz falló al generar la imagen.";
    }

    return "REP: Reporte generado correctamente.";
}

std::string FileSystemManager::imagenABase64(std::string path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";

    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});
    file.close();

    if (buffer.empty()) return "";

    static const char* b64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string res;
    int i = 0;
    unsigned char char_array_3[3], char_array_4[4];

    for (unsigned char c : buffer) {
        char_array_3[i++] = c;
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for(i = 0; i < 4; i++) res += b64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i) {
        int j;
        for(j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; j < i + 1; j++) res += b64_chars[char_array_4[j]];
        while(i++ < 3) res += '=';
    }
    return res;
}

int FileSystemManager::buscarEnCarpeta(int num_inodo, std::string nombre, FILE* file, Superbloque& sb) {
    Inodo inodo;
    fseek(file, sb.s_inode_start + (num_inodo * sizeof(Inodo)), SEEK_SET);
    fread(&inodo, sizeof(Inodo), 1, file);
    for (int i = 0; i < 15; i++) {
        if (inodo.i_block[i] != -1) {
            BloqueCarpeta bc;
            fseek(file, sb.s_block_start + (inodo.i_block[i] * 64), SEEK_SET);
            fread(&bc, 64, 1, file);
            for (int j = 0; j < 4; j++) {
                if (bc.b_content[j].b_inodo != -1 && std::string(bc.b_content[j].b_name) == nombre)
                    return bc.b_content[j].b_inodo;
            }
        }
    }
    return -1;
}

int FileSystemManager::buscarInodoPorPath(std::string path, FILE* file, Superbloque& sb) {
    if (path == "/" || path == "") return 0;
    std::vector<std::string> carpetas;
    std::stringstream ss(path);
    std::string item;
    while (std::getline(ss, item, '/')) if (!item.empty()) carpetas.push_back(item);
    int actual = 0;
    for (const std::string& s : carpetas) {
        actual = buscarEnCarpeta(actual, s, file, sb);
        if (actual == -1) return -1;
    }
    return actual;
}

int FileSystemManager::crearCarpeta(int inodo_padre, std::string nombre, FILE* file, Superbloque& sb) {
    int i_idx = encontrarBitLibre(sb.s_bm_inode_start, sb.s_inodes_count, file);
    marcarBit(sb.s_bm_inode_start, i_idx, '1', file);
    int b_idx = encontrarBitLibre(sb.s_bm_block_start, sb.s_blocks_count, file);
    marcarBit(sb.s_bm_block_start, b_idx, '1', file);

    Inodo nI;
    nI.i_s = 0; nI.i_type = '0'; nI.i_perm = 664;
    for(int i=0; i<15; i++) nI.i_block[i] = -1;
    nI.i_block[0] = b_idx;

    BloqueCarpeta bc;
    for(int i=0; i<4; i++) bc.b_content[i].b_inodo = -1;
    bc.b_content[0].b_inodo = i_idx; strcpy(bc.b_content[0].b_name, ".");
    bc.b_content[1].b_inodo = (inodo_padre == -1) ? i_idx : inodo_padre;
    strcpy(bc.b_content[1].b_name, "..");

    fseek(file, sb.s_inode_start + (i_idx * sizeof(Inodo)), SEEK_SET);
    fwrite(&nI, sizeof(Inodo), 1, file);
    fseek(file, sb.s_block_start + (b_idx * 64), SEEK_SET);
    fwrite(&bc, 64, 1, file);

    if (inodo_padre != -1) enlazarEnPadre(inodo_padre, i_idx, nombre, file, sb);
    return i_idx;
}

void FileSystemManager::enlazarEnPadre(int padre_idx, int hijo_idx, std::string nombre, FILE* file, Superbloque& sb) {
    Inodo padre;
    fseek(file, sb.s_inode_start + (padre_idx * sizeof(Inodo)), SEEK_SET);
    fread(&padre, sizeof(Inodo), 1, file);
    for (int i = 0; i < 15; i++) {
        if (padre.i_block[i] != -1) {
            BloqueCarpeta bc;
            fseek(file, sb.s_block_start + (padre.i_block[i] * 64), SEEK_SET);
            fread(&bc, 64, 1, file);
            for (int j = 0; j < 4; j++) {
                if (bc.b_content[j].b_inodo == -1) {
                    bc.b_content[j].b_inodo = hijo_idx;
                    strcpy(bc.b_content[j].b_name, nombre.c_str());
                    fseek(file, sb.s_block_start + (padre.i_block[i] * 64), SEEK_SET);
                    fwrite(&bc, 64, 1, file);
                    return;
                }
            }
        }
    }
}

int FileSystemManager::encontrarBitLibre(int start, int count, FILE* file) {
    char bit;
    for (int i = 0; i < count; i++) {
        fseek(file, start + i, SEEK_SET);
        fread(&bit, 1, 1, file);
        if (bit == '0') return i;
    }
    return -1;
}

void FileSystemManager::marcarBit(int start, int idx, char valor, FILE* file) {
    fseek(file, start + idx, SEEK_SET);
    fwrite(&valor, 1, 1, file);
}

std::string FileSystemManager::rep_tree(std::string id, std::string path) {

    std::string diskPath = "";
    int start = -1;

    for (auto &p : particionesMontadas) {
        if (p.id == id) {
            diskPath = p.path;
            start = p.start;
            break;
        }
    }

    if (diskPath == "") return "ERROR: ID no encontrado";

    FILE *file = fopen(diskPath.c_str(), "rb");
    if (!file) return "ERROR: No se pudo abrir disco";

    Superbloque sb;
    fseek(file, start, SEEK_SET);
    fread(&sb, sizeof(Superbloque), 1, file);

    std::string dot = "digraph G {\n";

    Inodo root;
    fseek(file, sb.s_inode_start, SEEK_SET);
    fread(&root, sizeof(Inodo), 1, file);

    dot += "inode0 [label=\"/\"];\n";

    int blockIndex = root.i_block[0];

    if (blockIndex != -1) {

        BloqueCarpeta carpeta;
        fseek(file, sb.s_block_start + blockIndex * sizeof(BloqueCarpeta), SEEK_SET);
        fread(&carpeta, sizeof(BloqueCarpeta), 1, file);

        for (int i = 0; i < 4; i++) {

            if (carpeta.b_content[i].b_inodo == -1) continue;

            std::string name = carpeta.b_content[i].b_name;

            if (name == "." || name == "..") continue;

            int inodeIndex = carpeta.b_content[i].b_inodo;

            dot += "inode" + std::to_string(inodeIndex) + " [label=\"" + name + "\"];\n";
            dot += "inode0 -> inode" + std::to_string(inodeIndex) + ";\n";
        }
    }

    dot += "}";

    fclose(file);

    std::ofstream out("/tmp/tree.dot");
    out << dot;
    out.close();

    std::string cmd = "dot -Tpng /tmp/tree.dot -o " + path;
    system(cmd.c_str());

    return "REPORTE TREE GENERADO";
}



extern std::vector<MountNode> particionesMontadas;

void recorrer(int index, FILE* file, Superbloque sb, std::stringstream &dot) {

    Inodo inodo;
    fseek(file, sb.s_inode_start + index * sizeof(Inodo), SEEK_SET);
    fread(&inodo, sizeof(Inodo), 1, file);

    std::string nombreNodo = "n" + std::to_string(index);

    // Nodo actual
    if(inodo.i_type == '0'){
        dot << nombreNodo << "[label=\"DIR " << index << "\"];\n";
    } else {
        dot << nombreNodo << "[label=\"FILE " << index << "\", shape=note];\n";
    }

    // Recorrer bloques
    for(int i = 0; i < 15; i++){

        if(inodo.i_block[i] == -1) continue;

        if(inodo.i_type == '0'){ // carpeta

            BloqueCarpeta bc;
            fseek(file, sb.s_block_start + inodo.i_block[i] * sizeof(BloqueCarpeta), SEEK_SET);
            fread(&bc, sizeof(BloqueCarpeta), 1, file);

            for(int j = 0; j < 4; j++){

                if(bc.b_content[j].b_inodo == -1) continue;

                std::string nombre = bc.b_content[j].b_name;

                if(nombre == "." || nombre == "..") continue;

                int hijo = bc.b_content[j].b_inodo;

                std::string nombreHijo = "n" + std::to_string(hijo);

                dot << nombreNodo << " -> " << nombreHijo 
                    << "[label=\"" << nombre << "\"];\n";

                recorrer(hijo, file, sb, dot);
            }
        }
    }
}

std::string FileSystemManager::rep_sb(std::string id, std::string path){

    std::string pathDisco = "";

    for(auto &p : particionesMontadas){
        if(p.id == id){
            pathDisco = p.path;
            break;
        }
    }

    if(pathDisco == "") return "ERROR: ID no encontrado";

    FILE* file = fopen(pathDisco.c_str(), "rb");
    if(!file) return "ERROR: No se pudo abrir disco";

    Superbloque sb;
    fseek(file, sizeof(MBR), SEEK_SET);
    fread(&sb, sizeof(Superbloque), 1, file);

    std::stringstream dot;

    dot << "digraph G {\n";
    dot << "node [shape=plaintext];\n";

    dot << "tabla [label=<\n";
    dot << "<table border='1' cellborder='1'>\n";

    dot << "<tr><td>Nombre</td><td>Valor</td></tr>\n";

    dot << "<tr><td>s_inodes_count</td><td>" << sb.s_inodes_count << "</td></tr>\n";
    dot << "<tr><td>s_blocks_count</td><td>" << sb.s_blocks_count << "</td></tr>\n";
    dot << "<tr><td>s_free_blocks_count</td><td>" << sb.s_free_blocks_count << "</td></tr>\n";
    dot << "<tr><td>s_free_inodes_count</td><td>" << sb.s_free_inodes_count << "</td></tr>\n";
    dot << "<tr><td>s_inode_start</td><td>" << sb.s_inode_start << "</td></tr>\n";
    dot << "<tr><td>s_block_start</td><td>" << sb.s_block_start << "</td></tr>\n";

    dot << "</table>\n>];\n}";
    
    fclose(file);

    std::string dotPath = "/tmp/sb.dot";
    std::ofstream out(dotPath);
    out << dot.str();
    out.close();

    std::string cmd = "dot -Tpng " + dotPath + " -o \"" + path + "\"";
    system(cmd.c_str());

    return "REPORTE SB GENERADO";
}

std::string FileSystemManager::rep_inode(std::string id, std::string path) {

    std::string log = "";

    std::string diskPath = "";
    int start = -1;

    for (auto &p : particionesMontadas) {
        if (p.id == id) {
            diskPath = p.path;
            start = p.start;
            break;
        }
    }

    if (diskPath == "") return "ERROR: ID no encontrado";

    FILE *file = fopen(diskPath.c_str(), "rb");
    if (!file) return "ERROR: No se pudo abrir disco";

    Superbloque sb;
    fseek(file, start, SEEK_SET);
    fread(&sb, sizeof(Superbloque), 1, file);

    std::string dot = "digraph G {\nnode [shape=plaintext];\n";

    for (int i = 0; i < sb.s_inodes_count; i++) {

        Inodo inodo;
        fseek(file, sb.s_inode_start + i * sizeof(Inodo), SEEK_SET);
        fread(&inodo, sizeof(Inodo), 1, file);

        if (inodo.i_block[0] == -1) continue;

        dot += "inode" + std::to_string(i) + " [label=<\n";
        dot += "<table border='1' cellborder='1'>\n";
        dot += "<tr><td colspan='2'>INODO " + std::to_string(i) + "</td></tr>\n";
        dot += "<tr><td>UID</td><td>" + std::to_string(inodo.i_uid) + "</td></tr>\n";
        dot += "<tr><td>GID</td><td>" + std::to_string(inodo.i_gid) + "</td></tr>\n";
        dot += "<tr><td>SIZE</td><td>" + std::to_string(inodo.i_s) + "</td></tr>\n";
        dot += "<tr><td>TYPE</td><td>" + std::string(1, inodo.i_type) + "</td></tr>\n";

        for (int j = 0; j < 15; j++) {
            dot += "<tr><td>block[" + std::to_string(j) + "]</td><td>" + std::to_string(inodo.i_block[j]) + "</td></tr>\n";
        }

        dot += "</table>>];\n";
    }

    dot += "}";

    fclose(file);

    std::ofstream out("/tmp/inode.dot");
    out << dot;
    out.close();

    std::string cmd = "dot -Tpng /tmp/inode.dot -o " + path;
    system(cmd.c_str());

    return "REPORTE INODE GENERADO";
}


std::string FileSystemManager::rep_block(std::string id, std::string path) {

    std::string diskPath = "";
    int start = -1;

    for (auto &p : particionesMontadas) {
        if (p.id == id) {
            diskPath = p.path;
            start = p.start;
            break;
        }
    }

    if (diskPath == "") return "ERROR: ID no encontrado";

    FILE *file = fopen(diskPath.c_str(), "rb");
    if (!file) return "ERROR: No se pudo abrir disco";

    Superbloque sb;
    fseek(file, start, SEEK_SET);
    fread(&sb, sizeof(Superbloque), 1, file);

    std::string dot = "digraph G {\nnode [shape=plaintext];\n";

    for (int i = 0; i < sb.s_blocks_count; i++) {

        BloqueCarpeta carpeta;

        fseek(file, sb.s_block_start + i * sizeof(BloqueCarpeta), SEEK_SET);
        fread(&carpeta, sizeof(BloqueCarpeta), 1, file);

        if (carpeta.b_content[0].b_inodo == -1) continue;

        dot += "block" + std::to_string(i) + " [label=<\n";
        dot += "<table border='1' cellborder='1'>\n";
        dot += "<tr><td colspan='2'>BLOCK " + std::to_string(i) + "</td></tr>\n";

        for (int j = 0; j < 4; j++) {
            dot += "<tr><td>" + std::string(carpeta.b_content[j].b_name) + "</td><td>" + std::to_string(carpeta.b_content[j].b_inodo) + "</td></tr>\n";
        }

        dot += "</table>>];\n";
    }

    dot += "}";

    fclose(file);

    std::ofstream out("/tmp/block.dot");
    out << dot;
    out.close();

    std::string cmd = "dot -Tpng /tmp/block.dot -o " + path;
    system(cmd.c_str());

    return "REPORTE BLOCK GENERADO";
}

