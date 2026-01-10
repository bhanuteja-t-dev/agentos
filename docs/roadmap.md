# AgentOS Roadmap

## Development Philosophy

AgentOS development follows an incremental, agent-centric approach. The focus is on evolving the agent model and security primitives rather than building traditional OS features like filesystems or networking. Each development cycle adds small, well-defined capabilities that enhance agent isolation, observability, and control.

## Completed Work

### Week 1: Bootable Kernel Foundation
- ✅ i386 Multiboot2 bootable kernel with GRUB
- ✅ VGA text-mode console (80x25) with cursor-based output
- ✅ Freestanding C toolchain setup (clang + lld, no libc)
- ✅ Basic build system (Makefile) with kernel, ISO, run, and debug targets
- ✅ GDB debugging setup with QEMU

### Week 2 Day 1: Agent Model and Security Foundations
- ✅ **Agent System**: Fixed-size agent table (16 agents) with lifecycle management (INVALID, CREATED, RUNNING, COMPLETED)
- ✅ **Intent-Based Execution**: Intent structure with action types and payloads, intent-to-capability mapping
- ✅ **Capability System**: Per-agent capability bitmasks (32-bit) with deny-by-default and explicit granting
- ✅ **Structured Audit Logging**: Append-only ring buffer (64 events) with structured records (type, result, agent_id, intent_action, sequence, message)
- ✅ **Intent Router**: Dynamic handler registry with O(1) lookup for extensible intent dispatch
- ✅ **Intent Handlers**: Concrete implementation for `INTENT_CONSOLE_WRITE` (VGA output)
- ✅ **Syscall Layer**: Capability-enforced `sys_intent_submit()` with comprehensive audit event emission
- ✅ **Security Model**: Complete audit trail of all security decisions (allow/deny) and agent actions

## Possible Future Work

The following items represent potential directions for AgentOS evolution. These are not commitments but rather explorations of how the agent model might evolve incrementally.

### Near-Term Explorations (Incremental Agent Model Evolution)

#### Agent Communication and Coordination
- **Agent-to-Agent Intents**: Enable agents to submit intents on behalf of other agents (with capability delegation)
- **Intent Channels**: Named communication channels where agents can send intents to other agents
- **Agent Discovery**: Query interface for agents to discover other agents by name or capability set

#### Enhanced Capability Management
- **Capability Revocation**: Ability to revoke previously granted capabilities at runtime
- **Time-Limited Capabilities**: Capabilities that expire after a certain duration or number of uses
- **Capability Delegation**: Agents can delegate (sub)capabilities to other agents with audit logging

#### Audit System Enhancements
- **Audit Query Interface**: Programmatic API for agents (with appropriate capabilities) to query audit logs
- **Audit Filtering**: Filter audit events by type, agent_id, intent_action, or time range
- **Audit Export**: Export audit log in structured formats (beyond console dump)

#### Intent System Extensions
- **Additional Intent Actions**: New intent types beyond `INTENT_CONSOLE_WRITE` (e.g., agent-to-agent messaging, capability query)
- **Intent Result Callbacks**: Allow agents to register callbacks for intent execution results
- **Composite Intents**: Structured way to combine multiple intent actions into atomic operations

#### Agent Lifecycle Refinements
- **Agent Persistence**: Save/restore agent state across kernel reboots
- **Agent Scheduling**: Preemptive or cooperative scheduling of multiple agents
- **Agent Termination**: Graceful agent shutdown with cleanup handlers
- **Agent Priority**: Priority levels for agent execution ordering

### Medium-Term Explorations (Security and Observability)

#### Enhanced Security Model
- **Capability Scoping**: Capabilities scoped to specific resources or operations (e.g., `CAP_CONSOLE_WRITE` for specific console regions)
- **Capability Hierarchies**: Hierarchical capability models where certain capabilities imply others
- **Agent Sandboxing**: Additional isolation mechanisms beyond capability checks (e.g., memory regions, execution limits)

#### Monitoring and Observability
- **Agent Health Checks**: Periodic health check intents to detect unresponsive agents
- **Resource Usage Tracking**: Track and audit resource consumption per agent (execution time, memory, intent submission rate)
- **Intent Statistics**: Aggregate statistics about intent submissions, success rates, and denial patterns

#### Control Plane Interface
- **Human-Readable Control Interface**: Intent-based interface for system administrators to query state, grant capabilities, and manage agents
- **Remote Audit Access**: Secure interface for external systems to read audit logs (with appropriate capabilities)

### Longer-Term Considerations (Uncertain Direction)

These areas are mentioned for completeness but are not current priorities. They may or may not align with the agent-first philosophy.

#### Storage and State
- **Persistent Agent State**: Agent-owned storage regions that persist across agent restarts
- **Shared State Regions**: Intent-based access to shared storage regions with capability-based access control
- **Note**: Traditional filesystem concepts (directories, file metadata, etc.) may not align with the intent-based model

#### Network and Communication
- **Network Intent Actions**: Intent-based network operations (send/receive) with capability enforcement
- **Remote Agent Communication**: Intents that can be submitted to agents running on remote AgentOS instances
- **Note**: Traditional network stacks may need adaptation to fit the intent-based security model

#### Development Tools and Experience
- **Agent Development Framework**: Tools and libraries for developing agents in higher-level languages
- **Agent Testing Framework**: Test harnesses for agents that simulate syscall responses
- **Debugging Tools**: Enhanced debugging support for agent execution (breakpoints in agent code, intent tracing)

## Development Principles

When evaluating potential features:

1. **Agent-Centric**: Does this feature enhance the agent model, agent isolation, or agent observability?
2. **Incremental**: Can this feature be added in small, well-defined steps?
3. **Security-First**: Does this feature maintain or enhance the security model (capability enforcement, audit logging)?
4. **Intent-Based**: Does this feature fit the intent-based execution model, or does it require direct syscall patterns?
5. **Observable**: Can all operations related to this feature be comprehensively audited?

## Current Focus

The immediate focus remains on:
- **Refining the existing agent model** based on experience using the current system
- **Exploring agent communication patterns** within the intent-based security model
- **Enhancing audit querying and analysis** capabilities for security monitoring
- **Incremental intent action extensions** as real agent use cases emerge

Traditional OS features (filesystems, device drivers, networking stacks) are explicitly **not** current priorities. The focus is on building a minimal, secure runtime for agents rather than a general-purpose operating system.
