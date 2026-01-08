// common/errors.h
#pragma once
#include <string>
#include <vector>

using namespace std;

struct ValidationError {
    string message;
    vector<string> related_ids;
};
