#pragma once
#include "ir.h"
#include <string>

using namespace std;

string writeIRToJsonFile(const IR& ir, const string& path);
