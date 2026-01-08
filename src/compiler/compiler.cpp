#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "NOT VALID\n";
        return 0;
    }

    std::ifstream in(argv[1]);
    if (!in) {
        std::cout << "NOT VALID\n";
        return 0;
    }

    try {
        json j;
        in >> j;

        // 🔹 Minimal validation for now
        if (j.is_object()) {
            std::cout << "VALID\n";
        } else {
            std::cout << "NOT VALID\n";
        }
    } catch (...) {
        std::cout << "NOT VALID\n";
    }

    return 0;
}
