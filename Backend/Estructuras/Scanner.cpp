#include "Scanner.h"
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <iterator>

using json = nlohmann::json;

std::vector<std::string> listaImagenes; 
// 🔥 IMPORTANTE: declarar vector global
extern std::vector<MountNode> particionesMontadas;

Scanner::Scanner() {
    // Constructor vacío
}

// 🔥 función externa
std::string convertirImagenABase64(const std::string& path);

std::string escapeJson(const std::string& input) {
    std::string output;

    for (char c : input) {
        switch (c) {
            case '\"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += c;
        }
    }

    return output;
}

std::string toLower(std::string str){
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

std::string Scanner::leerImagenBase64(std::string path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";

    std::vector<unsigned char> buffer(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    static const char encode[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string base64;
    int val = 0, valb = -6;

    for (unsigned char c : buffer) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            base64.push_back(encode[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6)
        base64.push_back(encode[((val << 8) >> (valb + 8)) & 0x3F]);

    while (base64.size() % 4)
        base64.push_back('=');

    return base64;
}

std::string Scanner::parse(std::string input) {

    std::stringstream full(input);
    std::string line;

    std::string log = "";
    std::vector<std::string> listaImagenes; // 🔥 AQUÍ EL CAMBIO CLAVE

    while (std::getline(full, line)) {

        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string command;
        ss >> command;
        command = toLower(command);

        int size = 0;
        std::string path="", name="", id="";

        std::string token;
        while (ss >> token) {
            size_t pos = token.find('=');
            if (pos == std::string::npos) continue;

            std::string param = token.substr(0,pos);
            std::string value = token.substr(pos+1);

            if(value.front()=='\"') value = value.substr(1,value.size()-2);

            if(param=="-size") size = stoi(value);
            else if(param=="-path") path = value;
            else if(param=="-name") name = value;
            else if(param=="-id") id = value;
        }

        try {

            if(command=="mkdisk"){
                log += diskManager.mkdisk(size,"m",path,'f') + "\n";
            }
            else if(command=="mkfs"){
                log += fileSystemManager.mkfs(id) + "\n";
            }

            else if(command=="fdisk"){
                log += diskManager.fdisk(size,"m",path,name,'p','f') + "\n";
            }

            else if(command=="mount"){
                log += diskManager.mount(path,name) + "\n";
            }

            else if(command=="rep"){

                std::string resultado = "";

                if(name == "disk"){

                    std::string pathDisco = "";

                    for(auto &p : particionesMontadas){
                        if(p.id == id){
                            pathDisco = p.path;
                            break;
                        }
                    }

                    if(pathDisco == ""){
                        log += "ERROR: ID no encontrado\n";
                        continue;
                    }

                    resultado = diskManager.rep_disk(pathDisco, path);
                } 
                else if(name == "tree"){
                    resultado = fileSystemManager.rep_tree(id, path);
                }
                else if(name == "sb"){
                    resultado = fileSystemManager.rep_sb(id, path);
                }
                else if(name == "inode"){
                    resultado = fileSystemManager.rep_inode(id, path);
                }
                else if(name == "block"){
                    resultado = fileSystemManager.rep_block(id, path);
                }
                else {
                    resultado = fileSystemManager.rep(name, path, id);
                }

                log += resultado + "\n";

                // 🔥 AQUÍ SE GUARDA CADA IMAGEN
                std::ifstream test(path);
                if(test){
                    std::string base64 = convertirImagenABase64(path);
                    if(!base64.empty()){
                        listaImagenes.push_back(base64); // 🔥 CLAVE
                    }
                } else {
                    log += "ERROR: No se generó imagen\n";
                }
            }

        } catch (...) {
            log += "ERROR ejecutando comando\n";
        }
    }

    // 🔥 RESPUESTA FINAL CORRECTA
    json respuesta;
    respuesta["log"] = log;
    respuesta["images"] = listaImagenes; // 🔥 DIRECTO

    return respuesta.dump();
}

std::string base64_chars = 
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

std::string base64_encode(const std::vector<unsigned char>& bytes) {
    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    for (unsigned char c : bytes) {
        char_array_3[i++] = c;
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (int j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];

        while (i++ < 3)
            ret += '=';
    }

    return ret;
}

std::string convertirImagenABase64(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";

    std::vector<unsigned char> buffer(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    return base64_encode(buffer);
}