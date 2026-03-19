#ifndef FILESYSTEMMANAGER_H
#define FILESYSTEMMANAGER_H

#include <string>
#include <vector>
#include <cstdio> 
#include "Estructuras.h"

struct MountNode {
    std::string id;
    std::string path;
    std::string name;
    int start;
    int size;
};

extern std::vector<MountNode> particionesMontadas; 

class FileSystemManager {
public:
    FileSystemManager();

    std::string mkfs(std::string id);
    std::string mkdir(std::string path, bool p);
    std::string mkfile(std::string path, int size, std::string cont);
    

    std::string cat(std::vector<std::string> files);
    
    std::string rep(std::string name, std::string path, std::string id);
    std::string imagenABase64(std::string path);

    std::string rep_tree(std::string id, std::string path);
    std::string rep_sb(std::string id, std::string path);
    std::string rep_inode(std::string id, std::string path);
    std::string rep_block(std::string id, std::string path);
    int buscarInodoPorPath(std::string path, FILE* file, Superbloque& sb);

private:
    int buscarEnCarpeta(int num_inodo, std::string nombre, FILE* file, Superbloque& sb);
    int crearCarpeta(int inodo_padre, std::string nombre, FILE* file, Superbloque& sb);
    void enlazarEnPadre(int padre_idx, int hijo_idx, std::string nombre, FILE* file, Superbloque& sb);

    int encontrarBitLibre(int start, int count, FILE* file);
    void marcarBit(int start, int idx, char valor, FILE* file);
};

#endif