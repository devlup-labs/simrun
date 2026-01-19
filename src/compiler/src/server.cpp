#include <crow.h>
#include <string>
#include "compiler_driver.h"

using namespace std;

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/compile").methods("POST"_method)
    ([](const crow::request& req) {

        bool ok = false;
        string output = compileIR(req.body, ok);

        if (!ok) {
            return crow::response(400, output);
        }

        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.body = output;
        return res;
    });

    app.port(8080).multithreaded().run();
}
