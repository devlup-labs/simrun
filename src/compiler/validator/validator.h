// validator/validator.h
#pragma once
#include "../ast/ast.h"
#include "../common/errors.h"
#include <vector>

using namespace std;

vector<ValidationError> validate_ast(const ArchitectureAST& ast);
