#include "validator.h"

string validateIR(const IR& ir) {

    if (ir.context.components.empty()) {
        return "IR must contain at least one component";
    }

    for (const auto& c : ir.context.components) {
        if (c.id <= 0) {
            return "Component id must be positive";
        }
        if (c.type.empty()) {
            return "Component type cannot be empty";
        }
    }

    return ""; // empty = no errors
}
