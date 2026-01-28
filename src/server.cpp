#include <crow.h>
#include <fstream>
#include <sstream>
#include "ir_parser.h"
#include "compiler_driver.h"

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/compile").methods("POST"_method)
([](const crow::request& req) {
    try {
        IR ir = parseIR(req.body);
        string path = compileIR(ir);

        ifstream in(path);
        stringstream ss;
        ss << in.rdbuf();

        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.body = ss.str();
        return res;

    } catch (const runtime_error& e) {
        // ALIDATION / USER ERROR
        return crow::response(
            400,
            string("Validation error: ") + e.what()
        );

    } catch (const exception& e) {
        // INTERNAL ERROR
        return crow::response(
            500,
            string("Internal compiler error: ") + e.what()
        );
    }
});


    app.port(8080).run();
}
