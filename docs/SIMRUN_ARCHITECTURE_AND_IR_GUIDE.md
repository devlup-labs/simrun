# SimRUN Architecture, IR, and Extension Guide

## 1) Purpose

This document explains the SimRUN implementation currently in `src/sim`:

- How the engine is structured.
- What each major file/module is responsible for.
- How simulation behavior is encoded and extended (including adding opcodes).
- How the JSON IR works and how to author it for larger systems.
- How design/debug logs are generated and used for analysis.

The codebase is deterministic discrete-event simulation (DES): event order is `(time, seq)`, with single RNG state and explicit logical cancellation.

---

## 2) High-Level Runtime Model

The engine compiles an immutable IR into runtime tables, then executes events until termination.

Core loop in `Simulator::Run(...)`:

1. Pop next event from scheduler heap.
2. Advance `sim_time`.
3. Integrate stats over elapsed interval.
4. Apply termination checks (horizon/predicate/step limit).
5. Discard stale events via token validity + request generation checks.
6. Dispatch event phase handler.
7. Repeat until queue empty or stop condition.

Design logs are buffered during execution and can be dumped to CSV at the end.

---

## 3) Code Structure and File Roles

### Build/Project

- `src/sim/CMakeLists.txt`
  - Defines `simrun_engine` library and `simrun_tests` binary.
  - Enables strict warning flags and sanitizer toggles.
- `src/sim/cmake/Sanitizers.cmake`
  - ASAN/UBSAN/TSAN wiring for non-MSVC toolchains.

### Core Types and Enums

- `src/sim/include/simrun/util/types.h`
  - Canonical ID/time aliases and invalid constants.
- `src/sim/include/simrun/core/enums.h`
  - `LocationType`, `TargetType`, `EventPhase`, `TokenState`, `QueueDiscipline`.
- `src/sim/include/simrun/core/event.h`
  - Event struct and lexical event comparator helpers.

### IR Layer

- `src/sim/include/simrun/ir/ir_schema.h`
  - Full in-memory IR schema (`SimulationIR` and all parts).
- `src/sim/include/simrun/ir/ir_validation.h`
  - Validation API.
- `src/sim/src/ir/ir_validation.cpp`
  - Structural/semantic checks:
    - contiguous IDs
    - routing completeness
    - behavior table completeness
    - bounds/sanity for workloads/bootstrap
- `src/sim/include/simrun/ir/json_ir_parser.h`
  - JSON parse/load API for IR.
- `src/sim/src/ir/json_ir_parser.cpp`
  - JSON parser + schema decoding into `SimulationIR`.
  - Supports enum/opcode names as strings.
  - Handles UTF-8 BOM in files.

### Runtime Data Stores

- `src/sim/include/simrun/runtime/request.h`
  - Runtime request token struct.
- `src/sim/include/simrun/runtime/request_table.h`
  - Request registry + payload slab API.
- `src/sim/src/runtime/request_table.cpp`
  - Contiguous request table, free stack, location invariants, payload blocks.
- `src/sim/include/simrun/runtime/token_table.h`
  - Logical cancellation token table.
- `src/sim/src/runtime/token_table.cpp`
  - Token allocation, invalidation, event validity checks.

### Scheduler

- `src/sim/include/simrun/scheduler/scheduler.h`
  - Binary-heap scheduler interface.
- `src/sim/src/scheduler/scheduler.cpp`
  - Min-heap behavior via comparator, global sequence assignment.

### Entity Runtime (Components/Links)

- `src/sim/include/simrun/entities/component_runtime.h`
  - Component runtime state and admission/preemption API.
- `src/sim/src/entities/component_runtime.cpp`
  - Waiting queue, slots, busy counters, slot tokens, preemption mechanics.
- `src/sim/include/simrun/entities/link_runtime.h`
  - Link runtime state and dispatch API.
- `src/sim/src/entities/link_runtime.cpp`
  - Transmission queue, `next_available_time`, in-flight tracking, dispatch.

### Behavior Execution

- `src/sim/include/simrun/behavior/opcodes.h`
  - Opcode enum + instruction representation.
- `src/sim/include/simrun/behavior/behavior_executor.h`
  - Behavior table and executor API.
- `src/sim/src/behavior/behavior_executor.cpp`
  - Behavior table build from IR and opcode interpreter.

### Execution/Stats/Simulation

- `src/sim/include/simrun/execution/execution_context.h`
  - Transient interpreter execution context.
- `src/sim/include/simrun/stats/stats_runtime.h`
  - Stats and design-trace APIs.
- `src/sim/src/stats/stats_runtime.cpp`
  - Histogram/integral updates, in-memory design trace, CSV dump.
- `src/sim/include/simrun/core/sim_context.h`
  - Global simulation context and build APIs.
- `src/sim/src/core/sim_context.cpp`
  - Build context from IR and from JSON IR file.
- `src/sim/include/simrun/core/simulator.h`
  - Simulator API (`Run(...)` etc.).
- `src/sim/src/core/simulator.cpp`
  - Main event loop, dispatch, workload/bootstrap injection, logging, finalize dump.

### Tests and Fixtures

- `src/sim/tests/simrun_tests.cpp`
  - Unit/integration coverage for scheduler, cancellation, determinism, logging, complex IR, JSON-equivalence.
- `src/sim/tests/fixtures/ir_format_template.json`
  - Minimal authoring template for IR.
- `src/sim/tests/fixtures/complex_system_ir.json`
  - Complex system IR fixture used by tests.

---

## 4) Behavior System and How to Add New Opcodes

### Current behavior pipeline

1. `opcode_stream` in IR stores linear instructions.
2. `behavior_bindings` map `(component_id, request_type, flag, phase)` to `instruction_pointer`.
3. At runtime, `BehaviorExecutor` finds pointer from `BehaviorTables`.
4. Interpreter executes instructions atomically for that event.

### Current opcode set

Defined in `opcodes.h`:

- `kHalt`
- `kSetFlag`
- `kIncrementHop`
- `kWritePayloadWord`
- `kAddComponentStateWord`
- `kScheduleTimer`
- `kSpawnRequest`
- `kInvalidateToken`
- `kRouteNextLink`
- `kLog`
- `kPreemptCurrent`

### Add a new opcode (step-by-step)

1. Add enum entry in `src/sim/include/simrun/behavior/opcodes.h`.
2. Implement behavior in interpreter switch:
   - `src/sim/src/behavior/behavior_executor.cpp`
3. Add JSON opcode mapping:
   - `ParseOpCode(...)` in `src/sim/src/ir/json_ir_parser.cpp`
4. If it introduces new invariants, extend IR validation:
   - `src/sim/src/ir/ir_validation.cpp`
5. Add tests:
   - Extend `src/sim/tests/simrun_tests.cpp` with focused and end-to-end coverage.

### Opcode design guidance

- Keep opcodes deterministic and side effects explicit.
- Avoid hidden global state; mutate only context-owned runtime tables.
- Ensure invalid input paths fail safely (drop/halt/throw as appropriate).
- Preserve event-causality: never schedule in the past.

### Adding new behavior (beyond just opcodes)

Adding an opcode is one part of behavior expansion, but not always enough.
Use this decision flow:

1. **Opcode-only change**
   - Use when behavior is expressible with existing runtime state and existing event phases.
   - Required files:
     - `src/sim/include/simrun/behavior/opcodes.h`
     - `src/sim/src/behavior/behavior_executor.cpp`
     - `src/sim/src/ir/json_ir_parser.cpp` (`ParseOpCode`)
     - tests in `src/sim/tests/simrun_tests.cpp`

2. **Behavior needs new IR config**
   - Use when behavior needs new parameters authored in JSON.
   - Also update:
     - `src/sim/include/simrun/ir/ir_schema.h`
     - `src/sim/src/ir/json_ir_parser.cpp`
     - `src/sim/src/ir/ir_validation.cpp`
     - JSON fixtures under `src/sim/tests/fixtures/`

3. **Behavior needs new runtime state**
   - Use when scripts need persistent fields not currently present.
   - Also update relevant runtime modules:
     - request state: `runtime/request*.h|cpp`
     - component state: `entities/component_runtime*.h|cpp`
     - link state: `entities/link_runtime*.h|cpp`
     - stats/log state: `stats/stats_runtime*.h|cpp`

4. **Behavior needs a new causal trigger**
   - Use when you need a new phase/lifecycle event.
   - Also update:
     - `src/sim/include/simrun/core/enums.h` (`EventPhase`)
     - `src/sim/src/behavior/behavior_executor.cpp` (`kPhaseCount`, phase handling)
     - `src/sim/src/ir/ir_validation.cpp` phase coverage checks
     - `src/sim/src/core/simulator.cpp` dispatch handling

5. **Behavior changes scheduling/cancellation semantics**
   - Also review:
     - `src/sim/src/scheduler/scheduler.cpp`
     - `src/sim/src/runtime/token_table.cpp`
     - `src/sim/src/core/simulator.cpp` (`ShouldDiscardEvent`, event scheduling paths)

### End-to-end behavior extension checklist

1. Define semantics and invariants first.
2. Implement opcode/runtime logic.
3. Extend JSON IR schema/parser/validation if needed.
4. Add design logs for observability (`kLog` or simulator-level log points).
5. Add tests:
   - unit-level behavior test
   - integration run with realistic IR fixture
   - deterministic equivalence check (same seed => same run result)
6. Verify with strict build and `simrun_tests`.

---

## 5) JSON IR Format

Top-level object fields:

- `global_parameters`
- `components`
- `links`
- `request_types`
- `routing_entries`
- `behavior_bindings`
- `opcode_stream`
- `workloads`
- `bootstrap_events`

### Enum strings accepted in JSON

- `discipline`: `"Fifo"`
- `phase`: `"RequestArrival"`, `"ServiceCompletion"`, `"TimerExpiration"`, `"ResponseArrival"`, `"WorkloadGeneration"`
- `opcode`: `"Halt"`, `"SetFlag"`, `"IncrementHop"`, `"WritePayloadWord"`, `"AddComponentStateWord"`, `"ScheduleTimer"`, `"SpawnRequest"`, `"InvalidateToken"`, `"RouteNextLink"`, `"Log"`, `"PreemptCurrent"`

The parser also accepts integer enum values, but string form is preferred for readability.

### Instruction format

Each opcode entry:

```json
{
  "opcode": "Log",
  "args": [2000, 0, 0, 0]
}
```

`args` is optional. Missing elements default to `0` (fixed 4-slot instruction args at runtime).

---

## 6) From System Design Diagram to IR

Use this process:

1. **Map nodes to components**
   - Each service node in your design becomes a `ComponentSpec`.
   - Choose capacity, queue limits, service time.

2. **Map edges to links**
   - Every directed path becomes a `LinkSpec` with propagation + bandwidth.

3. **Define request classes**
   - Distinct traffic types become `request_types`.
   - Use `flag_count` for behavior modes (e.g., priority, stateful branch).
   - Use `max_hops` as route progression depth.

4. **Define routing template**
   - Fill `routing_entries` for every `(request_type, flag, hop)`.
   - IR validation enforces completeness.

5. **Encode behavior**
   - Build scripts in `opcode_stream`.
   - Bind scripts to `(component, request_type, flag, phase)` in `behavior_bindings`.

6. **Define exogenous demand**
   - Use `workloads` for recurring generators.
   - Use `bootstrap_events` for initial conditions.

7. **Validate and run**
   - Parse JSON into `SimulationIR`.
   - Run validation before simulation.
   - Execute and inspect trace/stats.

---

## 7) Authoring IR for Big Systems

Recommended patterns:

- Keep canonical contiguous IDs for components/links/request types.
- Generate repetitive sections (`routing_entries`, `behavior_bindings`) from scripts/tools, not by hand.
- Use reusable opcode scripts and point multiple bindings to same instruction pointers.
- Keep `max_requests`/`max_tokens` conservative for burst-heavy workloads.
- Split scenario fixtures by purpose:
  - smoke/minimal fixture
  - stress fixture
  - regression fixture with known expected metrics.

For very large models, generate IR JSON from your system-design tooling pipeline, then load via `BuildSimContextFromJsonFile(...)`.

---

## 8) Logging and How It Helps Analysis

SimRUN has two logging channels:

1. **Design trace logs (structured)**
   - Stored in memory in `StatsRuntime`.
   - Optional CSV dump on run finalize.
   - Useful for:
     - timeline reconstruction
     - lifecycle tracking (`request_id`, phase, component)
     - event category analysis via `custom_fields`

2. **Debug console logs (human-readable)**
   - Printed by simulator when enabled.
   - Useful for:
     - development-time execution tracing
     - understanding queue/admission/dispatch behavior quickly

### Current toggles in `SimContext`

- `design_trace_logging_enabled`
- `debug_console_logging_enabled`
- `dump_design_trace_to_file_on_finish`
- `design_trace_output_path`

### Current CSV format

Header:

`time,request_id,event_phase,component_id,field0,field1`

`field0/field1` represent either simulator event codes or opcode-defined custom values.

---

## 9) Practical Usage

Load IR JSON and run:

```cpp
simrun::SimContext ctx = simrun::BuildSimContextFromJsonFile(
    "src/sim/tests/fixtures/complex_system_ir.json");
ctx.design_trace_logging_enabled = true;
ctx.debug_console_logging_enabled = false;
ctx.dump_design_trace_to_file_on_finish = true;
ctx.design_trace_output_path = "build/trace.csv";

simrun::Simulator sim(std::move(ctx));
simrun::RunResult r = sim.Run(50000);
```

---

## 10) Notes

- JSON parser supports integer-only numeric values for IR fields.
- IR validation is strict by design; missing routing/behavior matrix entries fail fast.
- Determinism is preserved by explicit IDs, event ordering, and centralized mutable state in `SimContext`.
