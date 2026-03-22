# src/sim Code Structure and Flow (Snapshot)

Date: 2026-03-21
Scope: `src/sim`

## 1) Exact Directory and File Structure

```text
src/sim
|-- core
|   |-- base_entity.h
|   |-- event_queue.cpp
|   |-- event_queue.h
|   |-- scheduler.h
|   |-- sim_types.h
|   |-- simulator.cpp
|   `-- simulator.h
|-- entities
|   |-- .gitkeep
|   |-- cache.cpp
|   |-- cache.h
|   |-- database.cpp
|   |-- database.h
|   |-- network_link.cpp
|   |-- network_link.h
|   |-- service.cpp
|   `-- service.h
|-- events
|   |-- .gitkeep
|   |-- api_events.cpp
|   |-- api_events.h
|   |-- database_events.cpp
|   |-- database_events.h
|   |-- event_header_file.h
|   |-- events.h
|   |-- network_link_event.cpp
|   `-- network_link_event.h
|-- factory
|   |-- .gitkeep
|   |-- factory.cpp
|   `-- factory.h
|-- logging
|   `-- .gitkeep
|-- src
|   |-- event_initializer.cpp
|   |-- event_initializer.h
|   |-- ir_parser.cpp
|   |-- ir_parser.h
|   |-- ir_types.h
|   `-- simulation_context.h
|-- e.json
`-- main.cpp
```

## 2) What Each Part Does

### `main.cpp`
- Defines the top-level simulation bootstrap flow:
1. Parse IR from input JSON path (`argv[1]`).
2. Build global `SimulationContext`.
3. Build entities and links via factories.
4. Apply initial state.
5. Register request types.
6. Seed initial events.
7. Run simulator.

### `src/` (IR and setup layer)
- `ir_types.h`: IR structs/enums (`IRHeader`, `IRComponent`, `IRLink`, `IRRequestType`, `IREvent`, etc.).
- `ir_parser.h/.cpp`: Parses JSON IR into `IR` struct.
- `simulation_context.h`: Global context container (seed, time unit, component map, link map, request types).
- `event_initializer.h/.cpp`: Converts `initial_events` into runtime events and schedules them.

### `core/` (engine loop primitives)
- `sim_types.h`: `using SimTime = uint64_t`.
- `event_queue.h`: Abstract event queue interface.
- `event_queue.cpp`: Priority queue implementation (earliest event time first).
- `scheduler.h`: Scheduler wrapper around queue (`schedule(...)`).
- `simulator.h/.cpp`: Main event loop (`pop`, advance time, execute event).
- `base_entity.h`: Shared entity base class (`id`).

### `factory/` (object creation)
- `factory.h/.cpp`: `EntityFactory` creates components/links, applies initial context, registers request types, and creates DB events.

### `entities/` (domain models)
- `service.h/.cpp`: Service node config/state (latency, concurrency, queue).
- `database.h/.cpp`: Database node config/state (seek model, token bucket, concurrency/queue, stats).
- `cache.h/.cpp`: Cache config/state (hit probability/latency, stats).
- `network_link.h/.cpp`: Link physics/state (bandwidth, loss, retries, queue, stats).

### `events/` (event behaviors)
- `events.h`: Minimal base event abstraction.
- `api_events.h/.cpp`: API request arrival/processing events.
- `database_events.h/.cpp`: DB queue/token handling, dispatch, send completion.
- `network_link_event.h/.cpp`: Link enqueue/departure, packet-loss logic, TCP retry/UDP forward.
- `event_header_file.h`: placeholder (empty).

### Other
- `e.json`: Sample IR input.
- `.gitkeep` files: folder placeholders.

## 3) General Runtime Flow (Intended)

1. `main.cpp` reads IR JSON.
2. IR parser builds typed `IR` object.
3. Context and factories are initialized.
4. Components and links are instantiated.
5. Initial component/link state is applied.
6. Request type metadata is loaded.
7. Initial events are converted to runtime events and scheduled.
8. Simulator loop repeatedly:
- pops earliest event,
- updates current simulation time,
- executes event logic,
- event logic may schedule further events.
9. Simulation ends when event queue is empty.

## 4) Event-Level Flow (Implemented Logic)

- DB flow:
1. `DBRequestArrivalEvent` checks tokens and concurrency.
2. If allowed, consumes tokens and schedules `DBRequestSendEvent` after sampled latency.
3. If blocked, queues request if space; else rejects.
4. `DBRequestSendEvent` decrements active count, refills tokens, dispatches next queued request if possible, then forwards request to next stage.

- Network flow:
1. `NetworkLinkArrivalEvent` enqueues packet/job and starts transmission if link idle.
2. `NetworkLinkDepartureEvent` computes packet loss.
3. On success, forwards to next event after propagation latency.
4. On UDP loss, marks degraded but forwards.
5. On TCP loss, schedules retry with exponential backoff.

## 5) Files Made

- In current `src/sim` code paths, there is no explicit file output/write pipeline implemented.
- Runtime side effect visible in code: console log output (`NetworkLogEvent`), not file generation.
- Documentation file added by this task: `docs/sim_code_structure.md`.

## 6) Current Codebase Status (Important)

`src/sim` currently contains mixed/in-progress interfaces. Key mismatches:

1. Include/path mismatches
- Many includes point to files/paths not present as written (`event.h`, `entity_factory.h`, `event_factory.h`, `request.h`, `rng.h`, `event_loop.h`, etc.).

2. Type/interface mismatches
- `Event` interface differs across files (`events.h` vs expected `event.h` shape with `timestamp/type/seed` and `execute(EventScheduler&)`).
- `Simulator` constructor/signature in `core/simulator.h` does not match how `main.cpp` constructs it.

3. Factory/API mismatches
- `main.cpp` calls `applyInitialComponentState` / `applyInitialLinkState`, while `factory` defines `applyComponentContext` / `applyLinkContext`.
- `EventInitializer` declaration and definition use different types (`EventFactory` vs `EntityFactory`, `Scheduler` signatures).

4. Constructor/signature mismatches in entities
- `database.h` constructor declaration does not match `database.cpp` definition.
- `BaseEntity` expects `std::string`, while entity constructors pass numeric IDs directly.

5. Parser unfinished points
- Event type mapping is incomplete (`type` used without definition in `ir_parser.cpp`).
- Component type decoding from JSON string to enum is not implemented (components currently pushed as `UNKNOWN`).

This means the architecture intent is clear, but the module boundaries and signatures need reconciliation before successful build/run.
