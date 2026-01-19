#pragma once
#include <string>
#include <map>
#include <variant>
#include <vector>

using namespace std;

using Value = variant<int, double, string, bool>;

struct ComponentIR {
    string id;
    string category;
    string implementation;

    map<string, Value> user_params;
    map<string, Value> resolved_params;
};

struct NetworkLinkIR {
    string from;
    string to;
    string type;

    map<string, Value> resolved_params;
};

struct IR {
    vector<ComponentIR> components;
    vector<NetworkLinkIR> links;
};
