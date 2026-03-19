#include "crow_all.h"
#include "FileSystemManager.h"
#include "DiskManager.h"
#include <iostream>
#include <string>
#include <vector>
#include "Scanner.h"

std::vector<MountNode> particionesMontadas;

struct CorsMiddleware {
    struct context {};

    void before_handle(crow::request&, crow::response&, context&) {}

    void after_handle(crow::request&, crow::response& res, context&) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    }
};

int main() {
    crow::App<CorsMiddleware> app;
    Scanner scanner;

    CROW_ROUTE(app, "/execute").methods("OPTIONS"_method)
    ([]() {
        return crow::response(204);
    });

CROW_ROUTE(app, "/execute").methods("POST"_method)
([&scanner](const crow::request& req) {

    auto x = crow::json::load(req.body);
    if (!x) {
        crow::response res;
        res.code = 400;
        res.set_header("Content-Type", "application/json");
        res.write("{\"log\":\"JSON inválido\"}");
        return res;
    }

    try {
        std::string comando = x["comando"].s();

        std::cout << ">>> COMANDO RECIBIDO:\n" << comando << std::endl;

        std::string response = scanner.parse(comando);

        std::cout << ">>> RESPUESTA:\n" << response << std::endl;

        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(response);

        return res;

    } catch (const std::exception& e) {
        std::cerr << "CRASH: " << e.what() << std::endl;

        crow::response res;
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write("{\"log\":\"Error interno\"}");

        return res;
    }
});

        CROW_ROUTE(app, "/<string>")
        ([](std::string filename){
            crow::response res;
            std::ifstream file("/home/yoshua/" + filename, std::ios::binary);

            if (!file) return crow::response(404);

            std::ostringstream contents;
            contents << file.rdbuf();

            res.write(contents.str());
            res.add_header("Content-Type", "image/png");
            return res;
        });

        CROW_ROUTE(app, "/<path>")
        ([](const crow::request&, crow::response& res, std::string path){
            std::ifstream file(path);
            if (!file.is_open()) {
                res.code = 404;
                res.end("No se encontró la imagen");
                return;
            }

            std::ostringstream contents;
            contents << file.rdbuf();
            res.write(contents.str());
            res.end();
        });

    std::cout << "====================================" << std::endl;
    std::cout << "   SERVIDOR C++ ACTIVO (CROW)" << std::endl;
    std::cout << "   URL: http://localhost:8080/execute" << std::endl;
    std::cout << "====================================" << std::endl;

    app.port(8080).multithreaded().run();
    return 0;
}

