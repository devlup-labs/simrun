#include "network_link.h"

NetworkLinkEntity::NetworkLinkEntity(uint32_t id, const nlohmann::json& params)
    : BaseEntity(id)
{
    /* -------- Topology -------- */
    from = params.at("from").get<std::string>();
    to = params.at("to").get<std::string>();

    /* -------- Physics Params -------- */
    bandwidth_mbps = params.at("bandwidth").get<double>();
    base_latency_mean = params.at("base_latency_mean").get<double>();
    base_packet_loss_rate = params.at("base_packet_loss_rate").get<double>();
    
    // Optional params with defaults
    tcp_rto_ms = params.value("tcp_rto", 200.0);
    mtu_bytes = params.value("mtu_bytes", 1500);

    /* -------- Initial Runtime State -------- */
    is_down = params.value("is_down", false);
    
    is_busy = false;
    current_packet_loss_rate = base_packet_loss_rate;
}