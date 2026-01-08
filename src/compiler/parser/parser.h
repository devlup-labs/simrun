// parser/parser.h
#pragma once
#include "../ast/ast.h"
#include <string>

using namespace std;

ArchitectureAST parse_json_to_ast(const string& json_input);
