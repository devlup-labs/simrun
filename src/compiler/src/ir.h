#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>

using namespace std;

using Value = variant<int, double, bool, string>;

struct Component {
    int id;
    string type;
    unordered_map<string, Value> config;
};

struct Link {
    int id;
    int from;
    int to;
    unordered_map<string, Value> config;
};

struct Context {
    vector<Component> components;
    vector<Link> links;
};

struct IR {
    unordered_map<string, Value> header;
    Context context;
};
