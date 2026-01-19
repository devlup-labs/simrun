#include "ir_parser.h"

using namespace std;

// TEMP: converts raw IR string into a fixed IR
IR parseIR(const string& raw) {
    IR ir;

    ComponentIR redis;
    redis.id = "cache1";
    redis.category = "cache";
    redis.implementation = "redis";
    redis.user_params["memory_gb"] = 2;

    NetworkLinkIR link;
    link.from = "server1";
    link.to = "cache1";
    link.type = "ethernet";

    ir.components.push_back(redis);
    ir.links.push_back(link);

    return ir;
}
