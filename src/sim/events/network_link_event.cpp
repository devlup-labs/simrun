void RequestArrivalAtLinkEvent::execute(Simulator& sim)
{
    auto& link = sim.get<NetworkLink>(link_id);

    /* ---------- queue handling ---------- */
    if (link.queue.size() >= link.queue_capacity) {
        request->mark_dropped();
        return;
    }

    link.queue.push(request);

    /* ---------- constant propagation latency ---------- */
    double propagation_latency = link.base_median_latency;

    /* ---------- serialization delay (bandwidth-limited) ---------- */
    double serialization_delay_ms =
        (link.packet_size_bytes * 8.0) /
        (link.bandwidth_mbps * 1e6) * 1000.0;

    double total_delay = propagation_latency + serialization_delay_ms;

    /* ---------- dequeue immediately (single-packet abstraction) ---------- */
    link.queue.pop();

    /* ---------- schedule arrival at destination component ---------- */
    sim.schedule_event(
        std::make_unique<RequestArrivalAtComponentEvent>(
            time + total_delay,
            dst_component_id,
            request
        )
    );
}
