#include <yaml-cpp/yaml.h>
#include <iostream>

using namespace std;

int main() {
    YAML::Node n = YAML::Load("a: 1");
    cout << n["a"].as<int>() << endl;
}
