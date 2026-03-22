Now I want architecture of the codebase and flow of the code (not using examples, but something that can be used by AI agents or interns to create the code base). Also provide a valid 

# ✔️ Conceptual Restatement (Confirming Understanding)

Your simulator has:

### Core runtime

* **Simulator**

  * event queue
  * global context
  * global mutable state
  * simulated clock

### State model

* Array of **components**
* Array of **links**
* Array of **request_types**

### Request model

Each **request_type** defines:

* id
* routing_table → 2-D matrix

  ```
  routing_table[flag][hop_idx] → link_id
  ```
* default_payload

Each **request instance** has:

* request_id
* pointer to request_type
* flag
* current_hop_idx
* payload

### Component behavior model

Each component stores:

```
behavioral_table[request_type][flag]
      → ordered list of behavioral_function_objects
```

Each behavioral function object:

```
{
   function_pointer,
   parameters
}
```

These functions:

* can mutate request
* can mutate component/link state
* can emit new events
* cannot advance simulated time

---

### Event execution philosophy

Instead of many specialized events:

* simulator has **few generic event types**
* behavior is driven by:

  * request_type
  * flag
  * component behavioral_table

Thus event loop is simple, but **behavioral richness comes from tables**.

Example event:

```
COMPONENT_EVENT(request_id)
```

Execution:

* locate request
* locate current component
* fetch behavior sequence
* execute functions sequentially

Example function:

```
change_flag(equal_prob, 0, 1)
send_to_link()
```

---

# ✔️ This Is a Valid Design Pattern

This is essentially:

> **A data-driven micro-behavior interpreter executed inside event handlers.**

Advantages:

* small number of event classes
* runtime configurable behavior
* easy extensibility
* compact IR

Costs:

* more indirection
* harder static optimization
* must be carefully specified to keep determinism

---

# Now: Formal Documentation of Execution Flow

Below is a **formal spec-style description** you can reuse.

---

# **SimRUN Behavioral Execution Model**

## 1. Runtime Data Structures

### 1.1 Simulator

Simulator maintains:

* event_queue
* simulation_clock
* global_context
* simulation_state

---

### 1.2 Simulation State

Simulation state contains:

* components[]
* links[]
* request_types[]
* active_requests[]

---

### 1.3 Request Type Definition

Each request_type is defined as:

```
RequestType {
    id
    routing_table : LinkID[][]     // indexed by flag, hop index
    default_payload
}
```

Routing table semantics:

```
routing_table[f][h] → link traversed at hop h
when request flag = f
```

---

### 1.4 Request Instance

```
Request {
    request_id
    request_type_id
    flag
    current_hop_idx
    payload
}
```

---

### 1.5 Component Behavioral Table

Each component maintains:

```
behavioral_table[request_type_id][flag]
      → sequence<BehaviorFunctionObject>
```

Where:

```
BehaviorFunctionObject {
    function_pointer
    parameters
}
```

Behavioral sequence is **ordered and deterministic**.

---

## 2. Event Model

Simulator supports a limited set of generic event types:

* COMPONENT_EVENT
* ARRIVE_AT_LINK
* ARRIVE_AT_COMPONENT
* WORKLOAD_EVENT
* FAULT_EVENT

Each event contains:

* timestamp
* event_type
* associated request_id (if applicable)
* target entity

---

## 3. COMPONENT_EVENT Execution Flow

When a COMPONENT_EVENT is dequeued:

### Step 1 — Extract Request Context

Simulator retrieves:

* request instance
* current component
* request_type
* request.flag
* request.current_hop_idx

---

### Step 2 — Resolve Behavioral Sequence

Behavioral sequence is obtained as:

```
seq =
component.behavioral_table
         [request.request_type_id]
         [request.flag]
```

---

### Step 3 — Sequential Behavioral Execution

For each function object in sequence:

```
for f in seq:
    execute f.function_pointer(
        simulator_context,
        request,
        f.parameters
    )
```

Each function may:

* mutate request state
* mutate component state
* mutate link state
* emit future events

Each function must NOT:

* change simulation clock
* directly manipulate event queue ordering
* inspect future events

---

## 4. Example: Load Balancer Routing

### Routing Table

```
flag 0 → [client-lb, lb-s0]
flag 1 → [client-lb, lb-s1]
```

### Behavioral Table Entry

```
request_type_x:
flag 0 →
   [
      { change_flag, { equal_prob, 0, 1 } },
      { send_to_link, {} }
   ]
```

---

### Execution Example

Initial:

```
request.flag = 0
request.current_hop_idx = 0
```

Execution:

#### change_flag()

* sample RNG
* possibly set request.flag = 1

#### send_to_link()

```
request.current_hop_idx++

link =
request_type.routing_table
        [request.flag]
        [request.current_hop_idx]

emit ARRIVE_AT_LINK(link, request)
```

Thus routing decision is **encoded in behavior + routing table**, not in event code.

---

## 5. Behavioral Function Constraints

Behavior functions must satisfy:

* deterministic given seed and state
* bounded execution
* no simulated-time advancement
* may emit only future events

Behavior functions form the **true semantic layer** of the simulation.

---

## 6. Overall Event Loop Semantics

Simulation progresses as:

```
while queue not empty:
    e = pop_min_time_event
    clock = e.time
    dispatch(e)
```

Behavioral logic is invoked only inside dispatch.

