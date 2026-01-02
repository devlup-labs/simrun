//For now, it converts IR → runtime objects

class Entity {
public:
    std::string id;
    explicit Entity(std::string id_) : id(std::move(id_)) {}    //explicit to prevent implicit type conversions
    virtual ~Entity() = default;    
};

//context defining
struct ServiceContext {
    int capacity;
    double latency_mean;
    double failure_prob;
};

struct DatabaseContext {
    int max_connections;
    double latency_mean;
    double failure_prob;
};

struct NetworkLinkContext {
    std::string from;
    std::string to;
    double latency_mean;
    double failure_prob;
};


//state definitions
struct ServiceState {
    bool is_down = false;
    int active_requests = 0;  //requests to be processed 
    int queued_requests = 0;  // active_requests > capacity → queue
};

struct DatabaseState {
    bool is_down = false;  
    int active_connections = 0; //database connections currently in use with services
};

struct NetworkLinkState {
    bool is_down = false;
    int in_flight = 0;          //requests sent but not arrived
};


//global containers for state and context
struct State {
    std::unordered_map<std::string, ServiceState> services;
    std::unordered_map<std::string, DatabaseState> databases;
    std::unordered_map<std::string, NetworkLinkState> links;
};

struct Context {
    std::unordered_map<std::string, std::unique_ptr<Entity>> entities;
    // read-only after factory
};



enum class IRType {
    SERVICE,
    DATABASE,
    NETWORK_LINK
};

struct IRNode {
    std::string id;
    IRType type;

    // connectivity
    std::string from;   // for network link
    std::string to;

    // generic parameters
    int capacity;

    double latency_mean;
    double failure_prob;
};

class ServiceEntity : public Entity {
public:
    ServiceContext ctx;
    ServiceEntity(std::string id, ServiceContext ctx_)
        : Entity(std::move(id)), ctx(ctx_) {}
};

class DatabaseEntity : public Entity {
public:
    DatabaseContext ctx;
    DatabaseEntity(std::string id, DatabaseContext ctx_)
        : Entity(std::move(id)), ctx(ctx_) {}
};

class NetworkLinkEntity : public Entity {
public:
    NetworkLinkContext ctx;
    NetworkLinkEntity(std::string id, NetworkLinkContext ctx_)
        : Entity(std::move(id)), ctx(std::move(ctx_)) {}
};


class EntityFactory {
public:
    void build(
        const std::vector<IRNode>& ir,
        Context& context,
        State& state,
        EventQueue& event_queue
    );

private:
    void create_context(
        const std::vector<IRNode>& ir,
        Context& context
    );

    void wire_links(
        const std::vector<IRNode>& ir,
        Context& context
    );

    void init_state_and_events(
        const Context& context,
        State& state,
        EventQueue& event_queue
    );
};


void EntityFactory::build(
    const std::vector<IRNode>& ir,
    Context& context,
    State& state,
    EventQueue& event_queue
) {
    create_context(ir, context);
    wire_links(ir, context);
    init_state_and_events(context, state, event_queue);
}

void EntityFactory::create_context(
    const std::vector<IRNode>& ir,
    Context& context
) {
    for (const auto& node : ir) {

        switch (node.type) {

        case IRType::SERVICE: {
            ServiceContext ctx {
                node.capacity,
                node.latency_mean,
                node.failure_prob
            };
            context.entities[node.id] =
                std::make_unique<ServiceEntity>(node.id, ctx);
            break;
        }

        case IRType::DATABASE: {
            DatabaseContext ctx {
                node.capacity,
                node.latency_mean,
                node.failure_prob
            };
            context.entities[node.id] =
                std::make_unique<DatabaseEntity>(node.id, ctx);
            break;
        }

        case IRType::NETWORK_LINK: {
            NetworkLinkContext ctx {
                node.from,
                node.to,
                node.latency_mean,
                node.failure_prob
            };
            context.entities[node.id] =
                std::make_unique<NetworkLinkEntity>(node.id, ctx);
            break;
        }

        }
    }
}

void EntityFactory::wire_links(
    const std::vector<IRNode>& ir,
    Context& context
) {
    for (const auto& node : ir) {
        if (node.type != IRType::NETWORK_LINK)
            continue;

        auto* link = static_cast<NetworkLinkEntity*>(
            context.entities.at(node.id).get()
        );

        context.entities.at(link->ctx.from);
        context.entities.at(link->ctx.to);
    }
}

void EntityFactory::init_state_and_events(
    const Context& context,
    State& state,
    EventQueue& eq
) {
    for (const auto& [id, entity] : context.entities) {

        if (dynamic_cast<ServiceEntity*>(entity.get())) {
            state.services[id] = ServiceState{};

            // Seed first request arrival
            eq.push(Event{
                .time = 0.0,
                .type = EventType::REQUEST_ARRIVAL,
                .target = id
            });
        }

        else if (dynamic_cast<DatabaseEntity*>(entity.get())) {
            state.databases[id] = DatabaseState{};
        }

        else if (dynamic_cast<NetworkLinkEntity*>(entity.get())) {
            state.links[id] = NetworkLinkState{};
        }
    }
}


