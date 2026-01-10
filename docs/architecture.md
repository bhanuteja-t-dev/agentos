# AgentOS Architecture

## High-Level Architecture

AgentOS is structured as a layered kernel with a clear separation of concerns. The system follows an agent-first design where agents (first-class citizens) declare their intentions through structured intents rather than directly invoking system operations. All security decisions are enforced through capability-based access control and comprehensively audited through a structured, append-only audit log.

The architecture consists of six major subsystems arranged in functional layers:

1. **VGA Console** - Bottom layer providing hardware abstraction for text-mode output
2. **Audit System** - Structured logging layer with append-only semantics
3. **Core Services Layer** - Agent lifecycle, Capability management, and Intent definitions
4. **Intent Router** - Dynamic handler dispatch mechanism
5. **Intent Handlers** - Concrete implementation of intent actions
6. **Syscall Layer** - Security enforcement and orchestration layer

The kernel entry point (`kernel_main`) initializes all subsystems in the correct order and coordinates agent creation, capability granting, and execution.

## Module Responsibilities

### VGA Console (`kernel/vga.c`, `kernel/vga.h`)

**Purpose**: Memory-mapped VGA text-mode driver providing console output abstraction.

**Responsibilities**:
- Direct hardware access to VGA text buffer at address 0xB8000
- Cursor-based output with automatic line wrapping and screen scrolling
- Newline character handling (`\n`) for multi-line output
- Screen clearing and cursor management

**Key Functions**:
- `vga_clear()` - Clear screen and reset cursor to top-left
- `vga_write(const char* s)` - Write null-terminated string with cursor advancement

**Dependencies**: None (standalone hardware abstraction layer)

**Design Notes**:
- Maintains global cursor state (row, column)
- Handles screen overflow by scrolling when reaching bottom
- Single responsibility: hardware I/O only, no business logic

---

### Audit System (`kernel/audit/audit.c`, `kernel/audit/audit.h`)

**Purpose**: Structured, append-only audit log for complete system traceability.

**Responsibilities**:
- Store all system events in a fixed-size ring buffer (64 events)
- Emit structured audit events with complete metadata
- Provide chronological ordering through sequence numbers
- Format and display audit log to VGA console on demand

**Key Data Structures**:
- `audit_event_t` - Structured record containing:
  - `audit_type_t type` - Event type (AGENT_CREATED, INTENT_SUBMIT, etc.)
  - `audit_result_t result` - Result (NONE, ALLOW, DENY, SUCCESS, FAILURE)
  - `agent_id_t agent_id` - Agent ID (-1 for system events)
  - `audit_intent_action_t intent_action` - Optional intent action (-1 if not applicable)
  - `unsigned int sequence` - Chronological sequence number
  - `char message[AUDIT_MSG_MAX]` - Contextual message string

**Key Functions**:
- `audit_init()` - Initialize ring buffer and emit initialization event
- `audit_emit(...)` - Append new event to ring buffer (append-only operation)
- `audit_dump_to_console()` - Read-only formatting and display of audit log

**Dependencies**:
- `vga.h` - For `audit_dump_to_console()` output
- `intent/intent.h` - For intent action value display (forward reference only)

**Design Principles**:
- **Append-Only**: `audit_emit()` is the only write operation; no modification or deletion functions exist
- **Structured Records**: All events stored as structured data, not plain strings
- **Chronological Ordering**: Sequence counter ensures correct temporal ordering even when buffer wraps
- **Immutable History**: Once written, audit events are never modified or deleted
- **View Layer Separation**: `audit_dump_to_console()` formats structured data on-the-fly for display; formatted strings are never stored in the audit buffer

**Ring Buffer Implementation**:
- Fixed-size array of 64 `audit_event_t` structures
- Write pointer (`audit_write_pos`) advances circularly
- Total event counter (`audit_total_count`) enables chronological reconstruction
- When buffer is full, oldest events are overwritten (oldest visible event is at `write_pos`)

---

### Agent System (`kernel/agent/agent.c`, `kernel/agent/agent.h`)

**Purpose**: Manage agent lifecycle as first-class kernel objects.

**Responsibilities**:
- Maintain fixed-size agent table (maximum 16 agents)
- Track agent states: INVALID, CREATED, RUNNING, COMPLETED
- Execute agent entry functions with context
- Emit audit events for all lifecycle transitions

**Key Data Structures**:
- `agent_t` - Agent structure containing:
  - `char name[AGENT_NAME_MAX]` - Agent identifier
  - `agent_entry_t entry` - Entry point function pointer
  - `void* context` - Context passed to entry function
  - `agent_state_t state` - Current lifecycle state

**Key Functions**:
- `agent_init()` - Initialize agent table
- `agent_create(name, entry, ctx)` - Create new agent, return agent ID
- `agent_run(id)` - Execute agent entry function and transition states
- `agent_count()` - Return number of created agents

**Dependencies**:
- `audit/audit.h` - For emitting lifecycle audit events

**Design Notes**:
- Agents are first-class citizens, not processes
- Sequential agent IDs (0-15) assigned by array index
- Entry functions receive context pointer for agent identification
- All state transitions are audited

---

### Capability System (`kernel/cap/cap.c`, `kernel/cap/cap.h`)

**Purpose**: Per-agent capability-based access control with deny-by-default.

**Responsibilities**:
- Maintain per-agent capability bitmasks (32-bit `uint32_t`)
- Grant capabilities explicitly (deny-by-default)
- Check capability presence for security enforcement
- Emit audit events for all capability grants

**Key Data Structures**:
- `cap_mask_t` - Capability bitmask type (unsigned int)
- Capability flags: `CAP_NONE` (0), `CAP_CONSOLE_WRITE` (0x00000001)

**Key Functions**:
- `cap_init()` - Initialize all agent capabilities to CAP_NONE
- `cap_grant(agent_id, mask)` - Grant capabilities to agent (OR operation)
- `cap_has(agent_id, mask)` - Check if agent has all specified capabilities (AND check)

**Dependencies**:
- `agent/agent.h` - For `AGENT_MAX_COUNT` constant
- `audit/audit.h` - For `agent_id_t` type and audit event emission

**Design Principles**:
- **Deny-by-Default**: All agents start with `CAP_NONE`
- **Explicit Granting**: Capabilities must be explicitly granted via `cap_grant()`
- **Fine-Grained**: Each intent action maps to specific required capabilities
- **Audited**: All capability grants emit audit events

---

### Intent System (`kernel/intent/intent.h`)

**Purpose**: Define intent-based execution model and capability mapping.

**Responsibilities**:
- Define intent action types as enumeration
- Define intent structure (action + payload)
- Map intent actions to required capabilities
- Provide type-safe intent definitions

**Key Data Structures**:
- `intent_action_t` - Enumeration of intent actions (e.g., `INTENT_CONSOLE_WRITE`)
- `intent_t` - Intent structure containing:
  - `intent_action_t action` - Intent action type
  - `char payload[INTENT_PAYLOAD_MAX]` - Fixed-size payload string (128 bytes)

**Key Functions**:
- `intent_action_to_capability(action)` - Map intent action to required capability mask

**Dependencies**:
- `cap/cap.h` - For `cap_mask_t` type

**Design Principles**:
- **Declarative Model**: Agents declare what they want (intent), not how to do it
- **Type Safety**: Intent actions are enumerated, not strings
- **Capability Mapping**: Each intent action explicitly maps to required capabilities
- **Extensible**: New intent actions can be added by extending the enum and capability mapping

---

### Intent Router (`kernel/intent/router.c`, `kernel/intent/router.h`)

**Purpose**: Dynamic handler registry for intent dispatch.

**Responsibilities**:
- Maintain handler function pointer table indexed by intent action
- Register handlers during kernel initialization
- Provide O(1) handler lookup
- Decouple syscall layer from intent execution logic

**Key Data Structures**:
- `intent_handler_t` - Function pointer type: `int handler(int agent_id, const intent_t* intent)`
- Fixed-size handler table indexed by `intent_action_t`

**Key Functions**:
- `intent_router_init()` - Initialize handler table (clear all entries)
- `intent_register_handler(action, handler)` - Register handler for intent action
- `intent_get_handler(action)` - Lookup handler for intent action (O(1))

**Dependencies**:
- `intent/intent.h` - For `intent_action_t` and `intent_t` types

**Design Principles**:
- **Dynamic Dispatch**: Handler lookup happens at runtime, enabling extensibility
- **Decoupling**: Syscall layer doesn't need action-specific logic
- **Extensibility**: New intent actions can be added without modifying syscall code
- **Fixed-Size Table**: No heap allocation, pre-allocated handler table

---

### Intent Handlers (`kernel/intent/handlers.c`, `kernel/intent/handlers.h`)

**Purpose**: Concrete implementations of intent actions.

**Responsibilities**:
- Implement actual operations for intent actions
- Execute hardware/system operations (e.g., VGA console output)
- Return success/failure status to syscall layer
- Maintain no knowledge of capabilities or audit (handled by syscall layer)

**Key Functions**:
- `handle_console_write(agent_id, intent)` - Print intent payload to VGA console

**Dependencies**:
- `intent/intent.h` - For `intent_t` type
- `vga.h` - For direct hardware access (handlers are allowed to call VGA)

**Design Principles**:
- **Single Responsibility**: Each handler implements one intent action
- **Hardware Access Allowed**: Handlers can directly access hardware (VGA) as they are trusted kernel code
- **No Security Logic**: Handlers assume capability checks have passed (enforced by syscall layer)
- **Stateless**: Handlers are pure functions with no internal state

---

### Syscall Layer (`kernel/syscall/syscall.c`, `kernel/syscall/syscall.h`)

**Purpose**: Security enforcement and orchestration of intent execution.

**Responsibilities**:
- Provide system call interface to agents
- Enforce capability-based security checks
- Coordinate intent validation, handler lookup, capability checking, and execution
- Emit comprehensive audit events for all security decisions

**Key Functions**:
- `sys_intent_submit(agent_id, intent)` - Primary intent submission interface:
  1. Validate intent structure and agent ID
  2. Emit `INTENT_SUBMIT` audit event
  3. Lookup handler via intent router
  4. Map intent action to required capability
  5. Check capability using `cap_has()`
  6. Emit `DENY` audit event if capability check fails
  7. Call handler function if capability check passes
  8. Emit `ALLOW` audit event on success or `FAILURE` on handler error
- `sys_console_write(agent_id, msg)` - Legacy syscall (agents should use intents)

**Dependencies**:
- `cap/cap.h` - For capability checking
- `vga.h` - For legacy `sys_console_write()` (not used in intent path)
- `audit/audit.h` - For comprehensive audit event emission
- `intent/intent.h` - For intent structures and capability mapping
- `intent/router.h` - For handler lookup

**Design Principles**:
- **Security Enforcement**: All security checks happen in syscall layer
- **Audit-First**: Every security decision is audited before execution
- **Orchestration**: Syscall layer coordinates multiple subsystems
- **No Action-Specific Logic**: Intent execution logic delegated to handlers via router

---

## Layering Rules

The following rules define which modules are allowed to call which other modules. These rules prevent circular dependencies and maintain clear architectural boundaries.

### Layer 0: Hardware Abstraction
- **VGA Console**: No dependencies (bottom layer)

### Layer 1: Core Services
- **Audit System**: 
  - Can call: VGA (for `audit_dump_to_console()`)
  - Cannot call: Agent, Capability, Intent, Syscall, Handlers
- **Agent System**: 
  - Can call: Audit
  - Cannot call: VGA (directly), Capability, Intent, Syscall, Handlers, Router
- **Capability System**: 
  - Can call: Audit
  - Cannot call: VGA (directly), Agent (except for `AGENT_MAX_COUNT` constant), Intent, Syscall, Handlers, Router

### Layer 2: Intent Definition
- **Intent System** (`intent.h`): 
  - Can call: Capability (for `cap_mask_t` type only)
  - Cannot call: Audit, Agent, VGA, Syscall, Handlers, Router
  - Note: This is a header-only module defining types and inline functions

### Layer 3: Intent Dispatch
- **Intent Router**: 
  - Can call: Intent (for types only)
  - Cannot call: Audit, Agent, Capability, VGA, Syscall, Handlers
- **Intent Handlers**: 
  - Can call: VGA (direct hardware access allowed), Intent (for types only)
  - Cannot call: Audit, Agent, Capability, Syscall, Router

### Layer 4: Security Enforcement
- **Syscall Layer**: 
  - Can call: Audit, Capability, Intent, Intent Router
  - Can call: Intent Handlers (indirectly via router lookup)
  - Cannot call: Agent (agents call syscalls, not vice versa), VGA (except legacy `sys_console_write()`)

### Layer 5: Orchestration
- **Kernel Main** (`main.c`): 
  - Can call: All modules for initialization and orchestration
  - Coordinates: Agent creation, capability granting, agent execution, audit dump

### Key Layering Constraints

1. **No Circular Dependencies**: The dependency graph is acyclic. Modules only depend on lower layers.

2. **Audit Isolation**: Audit system only calls VGA for display. Other modules call Audit but Audit never calls them back (prevents audit loops).

3. **Handler Privilege**: Intent handlers can directly access VGA because they are trusted kernel code. This privilege is granted only after capability checks in the syscall layer.

4. **Syscall as Security Boundary**: The syscall layer is the security enforcement point. No module above it (agents) can bypass capability checks.

5. **Intent Router Decoupling**: Intent router only provides lookup. It has no knowledge of capabilities, audit, or handlers. This enables extensibility.

6. **Agent Isolation**: Agent system only knows about Audit. Agents cannot directly call VGA, Capability, or Syscall (agents use syscall interface, not direct function calls from agent module).

---

## Rationale for Intent-Based Syscalls

AgentOS uses intent-based syscalls (`sys_intent_submit()`) instead of direct syscalls (e.g., `sys_console_write()`) for several architectural and security reasons:

### 1. **Declarative Security Model**
- **What vs. How**: Agents declare **what** they want to do (intent) rather than **how** to do it. This abstraction enables the kernel to make security decisions before execution.
- **Capability Mapping**: Each intent action maps to specific required capabilities. The kernel can validate capabilities without knowing implementation details of the operation.

### 2. **Auditability and Traceability**
- **Structured Intent Records**: Intent submission is audited with complete metadata (action type, payload, agent ID) before any execution occurs. This enables complete traceability of what agents attempted to do.
- **Pre-Execution Logging**: The `INTENT_SUBMIT` audit event is emitted before capability checks, ensuring that all intent submissions are logged regardless of outcome (allow or deny).

### 3. **Extensibility Without Syscall Modification**
- **Handler Registry**: New intent actions can be added by registering handlers in the intent router. The syscall layer (`sys_intent_submit()`) requires no modification to support new actions.
- **Decoupled Execution**: Intent execution logic lives in handlers, not in the syscall layer. This separation enables adding new intent types without modifying security enforcement code.

### 4. **Consistent Security Enforcement**
- **Single Enforcement Point**: All intent execution goes through `sys_intent_submit()`, which enforces capability checks consistently. There is no risk of bypassing security checks by calling different syscalls.
- **Centralized Audit**: All security decisions (allow/deny) are audited in one place with consistent event structure.

### 5. **Type Safety and Validation**
- **Enumerated Actions**: Intent actions are enumerated types (`intent_action_t`), not strings or magic numbers. This enables compile-time type checking and prevents invalid action types.
- **Structured Payload**: Intent payloads are fixed-size, type-safe structures. The kernel can validate intent structure before processing.

### 6. **Separation of Concerns**
- **Security Layer (Syscall)**: Handles validation, capability checking, and audit
- **Execution Layer (Handlers)**: Handles actual operation implementation
- **Policy Layer (Intent Router)**: Handles handler registration and dispatch

This separation allows security policies to be modified independently of handler implementations, and vice versa.

### Legacy Syscall Support
The system maintains `sys_console_write()` as a legacy syscall for backward compatibility, but new agent code should use `sys_intent_submit()` with `INTENT_CONSOLE_WRITE`. The intent-based model is the preferred interface going forward.

---

## Audit System: Append-Only and Structured Design

The audit system is designed with two fundamental principles: **append-only semantics** and **structured records**. These principles ensure audit integrity, complete traceability, and efficient querying.

### Append-Only Semantics

**Definition**: Once an audit event is written, it can never be modified or deleted. The audit log is an immutable, append-only history of all system actions.

**Implementation**:
- **Single Write Operation**: `audit_emit()` is the only function that writes to the audit buffer. No functions exist to modify or delete existing events.
- **Ring Buffer**: Fixed-size ring buffer (64 events) implements append-only semantics. When the buffer is full, the oldest events are overwritten by new events, but existing events are never modified in-place.
- **No Update Functions**: There are no `audit_modify()` or `audit_delete()` functions. The audit log grows forward in time only.

**Rationale**:
- **Tamper Resistance**: Append-only design prevents malicious code from modifying audit history to hide security violations.
- **Non-Repudiation**: Once an event is logged, it cannot be disputed or altered, providing strong non-repudiation guarantees.
- **Audit Integrity**: External audit systems can trust that events appear in the order they were written and have not been retroactively modified.

**Chronological Ordering**:
- **Sequence Numbers**: Each event has a monotonically increasing sequence number, enabling correct chronological ordering even when the ring buffer wraps around.
- **Total Count**: The system maintains `audit_total_count` to calculate which events are oldest when the buffer is full.
- **Reconstruction Algorithm**: `audit_dump_to_console()` reconstructs chronological order by calculating buffer positions based on sequence numbers and total count.

### Structured Records

**Definition**: Audit events are stored as structured data records with explicit fields for type, result, agent ID, intent action, sequence, and message. This contrasts with plain string logs that require parsing to extract information.

**Implementation**:
- **Structured Type**: `audit_event_t` is a C structure with explicit typed fields:
  ```c
  typedef struct {
      audit_type_t type;              // Event type enum
      audit_result_t result;          // Result enum
      agent_id_t agent_id;            // Agent ID (-1 for system)
      audit_intent_action_t intent_action; // Intent action (-1 if N/A)
      unsigned int sequence;          // Sequence number
      char message[AUDIT_MSG_MAX];    // Contextual message
  } audit_event_t;
  ```
- **Type Safety**: Event types and results are enumerations, not strings. This enables efficient filtering and type checking.
- **Explicit Metadata**: All relevant metadata (agent ID, intent action, result) is stored as structured fields, not embedded in message strings.

**Rationale**:
- **Query Efficiency**: Structured records enable efficient filtering and querying by type, agent ID, result, or intent action without parsing strings.
- **Programmatic Access**: External systems can programmatically access audit fields without regex parsing or string manipulation.
- **Type Safety**: Enumeration-based types prevent invalid event types or results from being stored.
- **Extensibility**: New event types or result values can be added to enumerations without breaking existing audit analysis tools.

**View Layer Separation**:
- **Formatting on Display**: `audit_dump_to_console()` formats structured records into human-readable strings **only when displaying**. Formatted strings are never stored in the audit buffer.
- **Multiple View Formats**: The same structured audit records could be formatted for different output formats (console, JSON, binary) without modifying the audit storage format.
- **Storage Efficiency**: Storing structured data is more space-efficient than storing pre-formatted strings for all possible display formats.

### Audit Event Lifecycle

1. **Emission**: Module calls `audit_emit()` with structured parameters (type, result, agent_id, intent_action, message).
2. **Validation**: `audit_emit()` validates parameters and prepares structured record.
3. **Storage**: Structured `audit_event_t` record is appended to ring buffer at current write position. Sequence number is assigned from `audit_total_count`.
4. **Advancement**: Write position advances (wraps around if buffer full). Total count increments.
5. **Retrieval**: `audit_dump_to_console()` reads structured records from buffer in chronological order.
6. **Formatting**: Records are formatted into human-readable strings on-the-fly for display (view layer).

### Security Properties

- **Integrity**: Append-only design ensures audit history cannot be tampered with after writing.
- **Completeness**: All security decisions (allow/deny) and system events are logged with structured metadata.
- **Traceability**: Every action can be traced back to the agent that initiated it via agent_id and intent_action fields.
- **Non-Repudiation**: Structured records with sequence numbers provide strong evidence that events occurred in a specific order.

---

## Design Principles Summary

1. **Security-by-Default**: Capabilities are denied by default; explicit granting required.
2. **Audit-First**: Every security decision is logged before execution.
3. **Intent-Based**: Agents declare intentions; kernel enforces security.
4. **Layered Architecture**: Clear module boundaries with explicit dependencies.
5. **Append-Only Audit**: Immutable audit history with structured records.
6. **Extensibility**: New intent actions can be added without modifying core security code.
7. **Type Safety**: Enumeration-based types prevent invalid operations.
8. **Separation of Concerns**: Security, execution, and policy are decoupled.
