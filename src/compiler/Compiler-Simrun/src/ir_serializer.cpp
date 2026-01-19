#include "ir_serializer.h"
#include <sstream>

using namespace std;

static string valueToString(const Value& v) {
    if (holds_alternative<int>(v)) return to_string(get<int>(v));
    if (holds_alternative<double>(v)) return to_string(get<double>(v));
    if (holds_alternative<bool>(v)) return get<bool>(v) ? "true" : "false";
    return "\"" + get<string>(v) + "\"";
}

string serializeIR(const IR& ir) {
    stringstream ss;

    ss << "{\n";
    ss << "  \"components\": [\n";

    for (size_t i = 0; i < ir.components.size(); ++i) {
        const auto& c = ir.components[i];

        ss << "    {\n";
        ss << "      \"id\": \"" << c.id << "\",\n";
        ss << "      \"category\": \"" << c.category << "\",\n";
        ss << "      \"implementation\": \"" << c.implementation << "\",\n";
        ss << "      \"resolved_params\": {\n";

        size_t cnt = 0;
        for (auto& [k, v] : c.resolved_params) {
            ss << "        \"" << k << "\": " << valueToString(v);
            if (++cnt < c.resolved_params.size()) ss << ",";
            ss << "\n";
        }

        ss << "      }\n";
        ss << "    }";
        if (i + 1 < ir.components.size()) ss << ",";
        ss << "\n";
    }

    ss << "  ],\n";
    ss << "  \"links\": [\n";

    for (size_t i = 0; i < ir.links.size(); ++i) {
        const auto& l = ir.links[i];

        ss << "    {\n";
        ss << "      \"from\": \"" << l.from << "\",\n";
        ss << "      \"to\": \"" << l.to << "\",\n";
        ss << "      \"type\": \"" << l.type << "\",\n";
        ss << "      \"resolved_params\": {\n";

        size_t cnt = 0;
        for (auto& [k, v] : l.resolved_params) {
            ss << "        \"" << k << "\": " << valueToString(v);
            if (++cnt < l.resolved_params.size()) ss << ",";
            ss << "\n";
        }

        ss << "      }\n";
        ss << "    }";
        if (i + 1 < ir.links.size()) ss << ",";
        ss << "\n";
    }

    ss << "  ]\n";
    ss << "}\n";

    return ss.str();
}
