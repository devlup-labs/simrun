#include "validator.h"
#include <iostream>

string validateIR(const IR& ir) {

    if (ir.context.components.empty()) {
        return "IR must contain at least one component";
    }

    for (const auto& c : ir.context.components) {
        std::cout << "  Validating component: id=" << c.id << ", type=" << c.type << std::endl;
        
        if (c.id <= 0) {
            std::cerr << "  ❌ Invalid component ID: " << c.id << " (must be > 0)" << std::endl;
            return "Component id must be positive";
        }
        if (c.type.empty()) {
            std::cerr << "  ❌ Invalid component type (empty)" << std::endl;
            return "Component type cannot be empty";
        }
        
        std::cout << "  ✅ Component valid" << std::endl;
    }

    return ""; // empty = no errors
}
