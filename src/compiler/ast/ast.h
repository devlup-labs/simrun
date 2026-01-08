// ast/ast.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

enum class ComponentType {
    API,
    DATABASE,
    CACHE,
    UNKNOWN
};

struct Component {
    string id;
    ComponentType type;
    string profile;

    // parameters (generic key-value for now)
    unordered_map<string, double> numeric_params;
};

struct Link {
    string source;
    string target;
    unordered_map<string, double> numeric_params;
};

struct Workload {
    string type;
    double base_rps;
    long duration_ms;
};

struct ArchitectureAST {
    unordered_map<string, Component> components;
    vector<Link> links;
    Workload workload;
};
