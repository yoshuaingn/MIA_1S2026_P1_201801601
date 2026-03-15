#include "crow_all.h"
#include <iostream>
#include <string>

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/execute").methods("POST"_method)([](const crow::request& req) {
        auto x = crow::json::load(req.body);
        crow::response res;
        
        // Habilitar CORS
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");

        if (!x) {
            res.code = 400;
            res.body = "{\"error\": \"JSON inválido\"}";
            return res; // Devuelve crow::response
        }

        std::string comando = x["comando"].s();
        std::cout << "Ejecutando: " << comando << std::endl;

        // Construimos el JSON
        crow::json::wvalue data;
        data["mensaje"] = "Comando recibido: " + comando;
        
        // Convertimos el JSON a string y lo metemos en el cuerpo de la respuesta
        res.body = data.dump(); 
        res.code = 200;
        
        return res; // Devuelve crow::response (¡Ahora coinciden los tipos!)
    });

    app.port(8080).multithreaded().run();
}