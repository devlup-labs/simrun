# **SimRUN DSL Compiler — Complete Implementation Plan**

## **Goal**

Build a **C++ compiler** that:

```
DSL Source  →  Parsed AST  →  Semantic Graph  →  Lowered IR Model  →  JSON IR
```

Guarantees:

* deterministic output
* complete IR generation
* future extensibility (DSL + VM opcode changes)
* modular clean codebase
* observability via logs

---

# **1. Compiler High Level Pipeline**

The compiler must be **multi-phase.**

Never merge phases.

```
1. Lexer
2. Parser → AST
3. Symbol Resolution
4. Semantic Validation
5. Workflow Lowering
6. Resource Model Synthesis
7. Routing Matrix Synthesis
8. Behavior Program Codegen
9. Workload + Bootstrap Generation
10. IR Assembly
11. Determinism Canonicalization
12. JSON Serialization
```

Each phase must be an independent module.

---

# **2. Project Directory Architecture**

This is critical.

```
src/compiler

  core/
    CompilerDriver.hpp
    CompilerDriver.cpp

  lexer/
    Token.hpp
    Lexer.hpp
    Lexer.cpp

  parser/
    AST.hpp
    Parser.hpp
    Parser.cpp

  semantic/
    SymbolTable.hpp
    SemanticAnalyzer.hpp
    SemanticAnalyzer.cpp

  lowering/
    WorkflowLowerer.hpp
    WorkflowLowerer.cpp
    ContinuationAllocator.hpp
    ContinuationAllocator.cpp

  ir/
    IRModel.hpp
    IRBuilder.hpp
    IRBuilder.cpp

  codegen/
    RoutingSynthesizer.hpp
    RoutingSynthesizer.cpp
    OpcodeGenerator.hpp
    OpcodeGenerator.cpp

  workloads/
    WorkloadBuilder.hpp
    WorkloadBuilder.cpp

  serialization/
    JsonEmitter.hpp
    JsonEmitter.cpp

  util/
    Logger.hpp
    Logger.cpp
    Determinism.hpp

  main.cpp
```

This ensures:

* VM change → edit codegen / ir only
* DSL change → edit parser / semantic only

---

# **3. Phase Details**

## **Phase 1 — Lexer**

Responsibilities:

* tokenize DSL
* preserve source location

Design:

```
enum class TokenType {
    Identifier,
    Number,
    Keyword,
    Symbol,
    End
};
```

Lexer must be:

* single-pass
* zero allocation per token if possible
* deterministic

---

## **Phase 2 — Parser (AST Construction)**

Use:

* recursive descent

AST must be **purely structural.**

Example:

```
struct ComponentDecl {
    std::string name;
    int capacity;
    AdmissionDecl admission;
    SchedulerDecl scheduler;
    ServiceTimeDecl serviceTime;
};
```

Parser must:

* never assign IDs
* never compute routing
* never validate cross references

Only build tree.

---

## **Phase 3 — Symbol Resolution**

Build global registry:

```
component_name → ComponentSymbol
request_type_name → RequestSymbol
program_name → ProgramSymbol
```

Assign **stable deterministic IDs**

Rule:

```
sort identifiers lexicographically
assign incremental ID
```

This guarantees reproducible IR.

---

## **Phase 4 — Semantic Validation**

Check:

* missing handlers
* invalid request references
* unreachable states
* infinite retry loops
* payload overflow
* queue capacity negative
* invalid distribution parameters

All errors must include:

```
file:line:column
```

---

## **Phase 5 — Workflow Lowering**

Convert high level constructs into:

> explicit state machine graph

Output structure:

```
struct LoweredHandler {
    vector<StateNode>
}
```

Insert:

* implicit response states
* timeout alternate edges
* retry loops

---

## **Phase 6 — Continuation Allocation**

Compute:

```
max_states_per_request_type
flag_dimensions
payload_words_required
```

Allocate:

```
continuation index bit width
retry counters
flags
```

Store mapping table:

```
(state_id → instruction_pointer)
```

---

## **Phase 7 — Resource Model Synthesis**

Create IR components:

```
IRComponent {
    id
    capacity
    queue_capacity
    scheduler
    service_time_model
}
```

Admission per class expands to:

```
waiting_queue_capacity = max(all classes)
```

OR future:

```
multi_queue IR extension
```

Keep compiler ready.

---

## **Phase 8 — Routing Matrix Synthesis**

For each request type:

Algorithm:

```
DFS workflow graph
assign hop index
expand routing flag branches
generate routing_entries
```

Output matches:

```
{
 request_type,
 flag,
 hop,
 link_id
}
```

---

## **Phase 9 — Opcode Generation**

Define VM-independent opcode layer:

```
enum class IRInstruction {
    Spawn,
    Suspend,
    SetContinuation,
    ScheduleTimer,
    Terminate,
    RNG,
    Halt
};
```

Then map to actual VM opcode string at serialization.

This allows:

* future opcode rename
* VM evolution

OpcodeGenerator must output:

```
vector<Opcode>
```

with instruction_pointer indexes.

---

## **Phase 10 — Behavior Binding Generation**

For each:

```
(component, request_type, flag, phase)
```

Bind correct instruction pointer.

Phases come from VM contract:

* RequestArrival
* ServiceCompletion
* TimerExpiration
* ResponseArrival
* WorkloadGeneration

Compiler must generate default no-op handler if not defined.

---

## **Phase 11 — Workload + Bootstrap**

Translate traffic DSL:

```
traffic HttpReq poisson(50/s)
```

into:

```
workloads[]
bootstrap_events[]
```

Bootstrap events include:

* first arrival
* periodic generators
* background jobs

---

## **Phase 12 — IR Assembly**

IRBuilder aggregates:

```
GlobalParameters
Components
Links
RequestTypes
RoutingEntries
BehaviorBindings
OpcodeStream
Workloads
BootstrapEvents
```

Validate:

* all IDs referenced exist
* no empty opcode stream
* routing completeness

---

## **Phase 13 — Determinism Pass**

Very important.

Sort:

* components by ID
* routing entries by tuple
* opcode stream stable
* workloads deterministic RNG stream

Hash DSL → embed build hash in debug logs.

---

## **Phase 14 — JSON Serialization**

JsonEmitter must:

* use streaming writer
* avoid floating nondeterminism
* format stable

Example:

```
writer.Key("components");
writer.StartArray();
```

Use:

* RapidJSON or nlohmann/json

---

# **4. Logging / Observability Strategy**

Logger should support:

```
LOG_INFO("Parsing component {}", name);
LOG_DEBUG("Allocated continuation states {}", n);
LOG_ERROR("Unknown request type {}", name);
```

Levels:

* INFO — phase progress
* DEBUG — internal allocation
* ERROR — semantic failures

Enable via CLI flag.

---

# **5. Compiler Driver**

Central orchestration:

```
CompilerDriver::compile(file) {

    tokens = Lexer.lex()
    ast = Parser.parse(tokens)
    symbols = SemanticAnalyzer.resolve(ast)
    lowered = WorkflowLowerer.lower(ast)
    cont = ContinuationAllocator.allocate(lowered)
    ir = IRBuilder.build(...)
    routing = RoutingSynthesizer.generate(...)
    opcodes = OpcodeGenerator.generate(...)
    workloads = WorkloadBuilder.generate(...)
    Determinism::canonicalize(ir)
    JsonEmitter.write(ir)
}
```

Driver must be:

* linear
* readable
* exception safe

---

# **6. Extensibility Design Principles**

To allow future VM / DSL evolution:

### ✔ never hardcode opcode strings in parser

### ✔ isolate IR schema in single header

### ✔ phase interfaces must be data-only

### ✔ use factory pattern for schedulers

### ✔ use variant/union for distribution models

### ✔ use visitor pattern for AST lowering

---

# **7. Testing Strategy**

Must include:

* golden DSL → golden IR tests
* determinism test (compile twice → diff)
* stress test (large workflows)
* semantic error tests

---

# **8. Build System**

Use:

```
CMake
- warnings as errors
- sanitizers
- clang-tidy
```

---

# **9. Immediate Next Step for Agent**

Agent should first implement:

1. IRModel + JsonEmitter
2. minimal Lexer + Parser
3. manual AST → IR mapping
4. then add lowering + routing

Iterative but architecture-first.