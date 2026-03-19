#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <string>
#include "Estructuras.h"
#include "FileSystemManager.h" 

class DiskManager {
public:
    DiskManager();
    std::string mkdisk(int size, std::string unit, std::string path, char fit);
    std::string rmdisk(std::string path);
    std::string fdisk(int size, std::string unit, std::string path, std::string name, char type, char fit);
    std::string rep_disk(std::string path, std::string output);
    
    
    std::string mount(std::string path, std::string name);
};

#endif