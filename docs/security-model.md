# AgentOS Security Model

## Threat Model

AgentOS assumes agents are potentially untrusted code that must be isolated from direct hardware access and kernel state modification. The security model is designed around the principle of least privilege: agents start with no capabilities and must be explicitly granted permissions to perform any system operations.

### What Agents Are Allowed

- **Execute Code**: Agents can execute their entry functions when invoked by the kernel
- **Submit Intents**: Agents can create and submit intent structures via `sys_intent_submit()` declaring what operations they want to perform
- **Receive Context**: Agents receive a context pointer (typically their agent ID) when their entry function is called

### What Agents Are Not Allowed

- **Direct Hardware Access**: Agents cannot directly access hardware resources (e.g., VGA memory at 0xB8000, I/O ports, memory-mapped devices)
- **Direct Kernel State Modification**: Agents cannot modify kernel data structures (agent tables, capability masks, audit buffers) except through the syscall interface
- **Bypass Capability Checks**: Agents cannot bypass the capability system or execute operations without appropriate permissions
- **Modify Audit Log**: Agents cannot read or modify the audit log (append-only, kernel-only access)
- **Direct Syscall Implementation**: Agents cannot implement their own syscalls or call kernel functions directly

All agent access to system resources must go through the syscall interface, which enforces capability-based security checks and comprehensive audit logging.

## Capability-Based Access Control

AgentOS implements fine-grained, per-agent capability-based access control with **deny-by-default** semantics. This model provides explicit security boundaries and enables least-privilege operation.

### Implementation

- **Initial State**: All agents start with `CAP_NONE` (no capabilities). Capabilities are explicitly denied by default.
- **Explicit Granting**: Capabilities must be explicitly granted via `cap_grant(agent_id, mask)` by kernel initialization code (or future control plane). Each grant is audited as a `USER_ACTION` with `SUCCESS` result.
- **Per-Agent Bitmasks**: Each agent has a 32-bit capability bitmask (`cap_mask_t`) storing granted capabilities. Capabilities are stored as bit flags (e.g., `CAP_CONSOLE_WRITE = 0x00000001`).
- **Fine-Grained Mapping**: Each intent action maps to a specific required capability. For example, `INTENT_CONSOLE_WRITE` requires `CAP_CONSOLE_WRITE`. The mapping is defined statically via `intent_action_to_capability()`.
- **Enforcement**: The syscall layer checks capabilities using `cap_has(agent_id, required_cap)` before executing any intent. This check requires **all** bits in the required capability mask to be present (AND operation).

### Rationale

Capability-based access control provides several security advantages over traditional discretionary access control:

1. **Principle of Least Privilege**: Agents start with no permissions and receive only the minimum capabilities needed for their function. This limits the damage if an agent is compromised.
2. **Explicit Permissions**: The security model makes privilege grants explicit and auditable. It is clear which agents have which capabilities at any point in time.
3. **Fine-Grained Control**: Capabilities can be granted at the granularity of individual operations (e.g., console write), enabling precise permission boundaries.
4. **Revocable Permissions**: While not yet implemented, the bitmask design enables future revocation by clearing specific capability bits.
5. **Non-Transitive**: Capabilities are per-agent and do not automatically propagate. An agent cannot grant its own capabilities to other agents (only kernel code can call `cap_grant()`).

## Intent-Based APIs and Attack Surface Reduction

AgentOS uses intent-based syscalls where agents declare **what** they want to do rather than **how** to do it. This abstraction significantly reduces the attack surface by limiting the operations available to agents.

### Intent Submission Model

Agents create structured `intent_t` objects specifying:
- **Action Type**: Enumerated intent action (e.g., `INTENT_CONSOLE_WRITE`). Actions are type-safe enumerations, not strings or magic numbers.
- **Payload**: Fixed-size payload string (128 bytes) containing operation-specific data.

Agents submit intents via `sys_intent_submit(agent_id, intent)`, which:
1. Validates the intent structure (action type, agent ID)
2. Emits an `INTENT_SUBMIT` audit event **before** any capability checks
3. Looks up the handler for the intent action
4. Maps the intent action to its required capability
5. Checks if the agent has the required capability
6. Executes the handler only if the capability check passes
7. Emits `ALLOW` or `DENY` audit events for the security decision

### Attack Surface Reduction

Intent-based APIs reduce the attack surface in several ways:

1. **Limited Operation Set**: Agents can only perform operations that have been defined as intent actions. There is no generic "do anything" syscall interface. New operations require kernel changes (new intent types and handlers).
2. **Type Safety**: Intent actions are enumerated types checked at compile time. Agents cannot submit invalid or unknown action types that might trigger undefined behavior.
3. **Structured Validation**: Intent structures have fixed layouts. The kernel validates intent structure before processing, preventing malformed or oversized payloads from causing buffer overflows.
4. **Single Enforcement Point**: All intent execution goes through `sys_intent_submit()`, providing a single point of security enforcement. There is no risk of bypassing security checks by calling different syscalls.
5. **Separation of Policy and Implementation**: Intent handlers are separated from security enforcement. Handlers are trusted kernel code, but they are only called after capability checks. This reduces the risk of security bugs in handler implementations affecting the security model.
6. **Pre-Execution Auditing**: Intent submission is audited before capability checks, ensuring that all attempted operations are logged regardless of outcome. This provides complete visibility into agent behavior.

## Handling of Denied Actions

When an agent attempts to perform an action without the required capability, the syscall layer denies the operation and emits a comprehensive audit event. Denied actions are handled consistently and securely.

### Denial Process

1. **Capability Check Failure**: `sys_intent_submit()` calls `cap_has(agent_id, required_cap)` and receives a negative result (the agent does not have all required capability bits).

2. **Audit Event Emission**: The syscall layer immediately emits an audit event with:
   - Type: `AUDIT_TYPE_SYSTEM_ERROR`
   - Result: `AUDIT_RESULT_DENY`
   - Agent ID: The agent that attempted the operation
   - Intent Action: The intent action that was denied
   - Message: The intent payload (for context)

3. **Error Return**: The syscall returns `-1` to indicate failure. No side effects occur.

4. **Agent Handling**: The agent receives the error return code and can choose how to handle the denial (e.g., retry with different parameters, abort operation, report error to higher layer). The kernel does not impose any specific denial handling behavior on agents.

### Security Properties of Denials

- **Fail-Safe**: Denials fail securely by returning an error without performing any operation. There is no partial execution or state corruption.
- **Audited**: All denials are logged before the error is returned, ensuring complete audit trail even if the agent crashes or aborts immediately after denial.
- **Non-Leaking**: The denial audit event includes the intent payload, which may contain sensitive information. This is acceptable because audit logs are append-only and kernel-protected, but future versions may implement payload sanitization for certain intent types.
- **Consistent**: Denials are handled identically for all intent actions, providing predictable security behavior across the system.

## Audit Logging as First-Class Security

Audit logging is not an afterthought in AgentOS; it is a fundamental security primitive that enables accountability, forensics, and security monitoring. The audit system is designed to be comprehensive, tamper-resistant, and queryable.

### Audit-First Design

Every security decision in AgentOS is audited **before** execution occurs:

1. **Intent Submission**: `INTENT_SUBMIT` events are emitted immediately when `sys_intent_submit()` is called, before capability checks. This ensures all intent attempts are logged, regardless of allow/deny outcome.

2. **Security Decisions**: Both `ALLOW` and `DENY` decisions are audited with structured records including:
   - Event type (`USER_ACTION` for allows, `SYSTEM_ERROR` for denials)
   - Result (`ALLOW` or `DENY`)
   - Agent ID (which agent made the request)
   - Intent action (what operation was attempted/allowed/denied)
   - Payload context (what data was involved)

3. **Capability Grants**: All capability grants are audited as `USER_ACTION` with `SUCCESS` result, including the granted capability mask and recipient agent ID.

4. **Agent Lifecycle**: All agent state transitions (creation, start, completion, errors) are audited with appropriate event types.

### Why Audit is First-Class

1. **Complete Traceability**: The audit log provides a complete, chronological record of all security decisions and agent actions. Security analysts can trace any system behavior back to the originating agent and intent.

2. **Non-Repudiation**: Append-only, structured audit records provide strong evidence that events occurred in a specific order. Agents cannot dispute their actions when audit records show clear intent submission and execution.

3. **Forensics**: In the event of a security incident, the audit log enables forensic analysis to determine what happened, which agents were involved, and how the incident unfolded over time.

4. **Security Monitoring**: Structured audit records enable programmatic analysis and filtering. Security monitoring systems can query audit logs for specific patterns (e.g., all denials for a specific agent, all capability grants, all console write operations).

5. **Tamper Resistance**: Append-only design prevents malicious code from modifying audit history to hide security violations. Once an event is logged, it cannot be altered or deleted.

6. **Compliance and Accountability**: The comprehensive audit trail enables compliance with security requirements and provides accountability for agent actions. System administrators can demonstrate that security policies were enforced correctly.

### Structured Audit Records

Audit events are stored as structured records with explicit fields (type, result, agent_id, intent_action, sequence, message) rather than plain text logs. This design enables:

- **Efficient Querying**: Programs can filter and query audit logs by type, agent ID, result, or intent action without parsing strings.
- **Type Safety**: Event types and results are enumerations, preventing invalid or inconsistent audit records.
- **Programmatic Access**: External security systems can programmatically analyze audit logs without regex parsing or string manipulation.
- **Multiple View Formats**: The same structured records can be formatted for different outputs (console, JSON, binary) without modifying the storage format.

### Audit Coverage

The audit system logs:
- **System Initialization**: Boot events and subsystem initialization
- **Agent Lifecycle**: Creation, execution start, completion, errors
- **Capability Management**: All capability grants (explicitly audited)
- **Intent Submissions**: All intent submissions, regardless of outcome
- **Security Decisions**: All allow/deny decisions with complete context
- **Execution Results**: Handler success/failure outcomes

This comprehensive coverage ensures that security-relevant events cannot occur without leaving an audit trail.
