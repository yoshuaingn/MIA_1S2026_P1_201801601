#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include "DiskManager.h"
#include "FileSystemManager.h"

class Scanner {
public:
    Scanner();
    std::string parse(std::string input);
    std::string leerImagenBase64(std::string path);

private:
    DiskManager diskManager;
    FileSystemManager fileSystemManager;
};

#endif