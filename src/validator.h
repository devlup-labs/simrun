#pragma once
#include "ir.h"
#include <string>

using namespace std;

// Returns empty string if valid, otherwise error message
string validateIR(const IR& ir);
