#include <crow.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "ir_parser.h"
#include "compiler_driver.h"

using json = nlohmann::json;

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/compile").methods("POST"_method)
([](const crow::request& req) {
    try {
        std::cout << "\n=== COMPILER REQUEST RECEIVED ===" << std::endl;
        std::cout << "Raw body: " << req.body << std::endl;
        
        json requestBody = json::parse(req.body);
        std::cout << "Parsed request successfully" << std::endl;
        
        string irJson = requestBody["project"].dump();
        std::cout << "Extracted project field: " << irJson << std::endl;
        
        IR ir = parseIR(irJson);
        std::cout << "Parsed IR - Components count: " << ir.context.components.size() << std::endl;
        std::cout << "Parsed IR - Links count: " << ir.context.links.size() << std::endl;
        
        for (size_t i = 0; i < ir.context.components.size(); i++) {
            std::cout << "  Component " << i << ": id=" << ir.context.components[i].id 
                      << ", type=" << ir.context.components[i].type << std::endl;
        }
        
        string path = compileIR(ir);
        std::cout << "Compilation successful! Output: " << path << std::endl;

        ifstream in(path);
        stringstream ss;
        ss << in.rdbuf();

        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.body = ss.str();
        std::cout << "Sending response (HTTP 200)" << std::endl;
        return res;

    } catch (const runtime_error& e) {
        // VALIDATION / USER ERROR
        std::cerr << "VALIDATION ERROR: " << e.what() << std::endl;
        return crow::response(
            400,
            string("Validation error: ") + e.what()
        );

    } catch (const exception& e) {
        // INTERNAL ERROR
        std::cerr << "INTERNAL ERROR: " << e.what() << std::endl;
        return crow::response(
            500,
            string("Internal compiler error: ") + e.what()
        );
    }
});


    app.port(8081).run();
}
