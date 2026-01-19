#include "validator.h"

using namespace std;

string validateIR(const IR& ir) {
    if (ir.components.empty()) {
        return "No components defined in IR";
    }

    for (auto& c : ir.components) {
        if (c.implementation.empty()) {
            return "Component implementation missing";
        }
    }

    return ""; // valid
}
