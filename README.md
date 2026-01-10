# AgentOS

An agent-first operating system designed to run long-lived AI agents safely with strict capability controls, intent-based APIs, and comprehensive audit logging. AgentOS treats agents as first-class citizens with explicit security boundaries enforced through capability-based access control. Unlike traditional operating systems where processes have broad permissions, agents in AgentOS declare their intentions through structured intents, which are validated against fine-grained capabilities before execution. All system actions are recorded in a structured audit log, providing complete traceability of agent behavior and security decisions.

AgentOS is built as a minimal freestanding kernel using freestanding C (no libc), targeting i386 architecture with Multiboot2 boot protocol. The system emphasizes security-by-default: agents must explicitly declare their intent to perform actions, capabilities are denied by default and granted explicitly, and every security decision is logged with structured audit records including event type, result, agent ID, intent action, and contextual messages.

## Architecture Overview

### Agent System
Fixed-size agent table (maximum 16 agents) managing agent lifecycle. Each agent has a name, entry function, context pointer, and state (INVALID, CREATED, RUNNING, COMPLETED). Agents are created with `agent_create()` and executed via `agent_run()`, which calls the agent's entry function with its context. All agent lifecycle events (creation, start, completion) are emitted to the audit log.

### Intent System
Intent-based execution model where agents declare what they want to do rather than directly calling system functions. An intent consists of an action type (e.g., `INTENT_CONSOLE_WRITE`) and a fixed-size payload string. The system maps intent actions to required capabilities, enabling capability-based access control at the intent level.

### Intent Router
Handler registry for dynamic intent dispatch. Handlers are registered during kernel initialization, mapping intent actions to handler functions. The router provides O(1) lookup of handlers and decouples intent execution logic from the syscall layer, enabling extensible intent handling without modifying syscall code.

### Capability System
Per-agent capability bitmask (32-bit) implementing fine-grained access control. Capabilities are denied by default and must be explicitly granted via `cap_grant()`. Each intent action requires specific capabilities (e.g., `INTENT_CONSOLE_WRITE` requires `CAP_CONSOLE_WRITE`). The syscall layer enforces capability checks before executing intents, auditing both allow and deny decisions.

### Audit System
Structured audit log implemented as a fixed-size ring buffer (64 events) storing complete records of all system actions. Each audit event includes: event type (AGENT_CREATED, INTENT_SUBMIT, SYSTEM_ERROR, etc.), result (NONE, ALLOW, DENY, SUCCESS, FAILURE), agent ID, optional intent action, sequence number for chronological ordering, and a message string. Events are emitted throughout the system lifecycle, providing complete traceability of agent behavior and security decisions. The audit log can be dumped to the VGA console in chronological order.

### Syscall Layer
System call interface enforcing capability-based security. The primary entry point is `sys_intent_submit()`, which validates the intent, looks up the handler, checks required capabilities, executes the handler, and emits structured audit events for each step. A legacy `sys_console_write()` syscall is maintained but agents are expected to use intent-based APIs.

### VGA Console
Memory-mapped VGA text-mode driver (80x25) providing cursor-based console output with automatic line wrapping and screen scrolling. Supports newline handling and maintains global cursor state. The console serves as the audit log output destination when `audit_dump_to_console()` is called.

## Execution Flow

The following flow describes how an agent submits an intent and how it is processed:

1. **Agent Creation**: Agent is created with a name, entry function, and context. The agent system allocates a slot in the fixed-size table and emits an `AGENT_CREATED` audit event.

2. **Capability Granting**: Required capabilities are explicitly granted to agents via `cap_grant()`. Each grant is audited as a `USER_ACTION` with `SUCCESS` result.

3. **Agent Execution**: Agent is executed via `agent_run()`, which transitions the agent to `RUNNING` state, emits an `AGENT_STARTED` audit event, and calls the agent's entry function.

4. **Intent Creation**: The agent entry function creates an `intent_t` structure specifying the action type (e.g., `INTENT_CONSOLE_WRITE`) and payload data.

5. **Intent Submission**: Agent calls `sys_intent_submit()` with its agent ID and the intent structure. The syscall layer immediately emits an `INTENT_SUBMIT` audit event with the intent action and payload.

6. **Handler Lookup**: The syscall layer looks up the registered handler for the intent action via the intent router. If no handler is registered, an audit `SYSTEM_ERROR` with `FAILURE` result is emitted and the syscall returns.

7. **Capability Mapping**: The system maps the intent action to its required capability (e.g., `INTENT_CONSOLE_WRITE` → `CAP_CONSOLE_WRITE`) using `intent_action_to_capability()`.

8. **Capability Enforcement**: The syscall layer checks if the agent has the required capability using `cap_has()`. If the check fails, an audit `SYSTEM_ERROR` with `DENY` result is emitted (including the intent action) and the syscall returns.

9. **Handler Execution**: If the capability check passes, the handler function is called with the agent ID and intent. Handlers execute the actual operation (e.g., printing to VGA console).

10. **Execution Result**: If the handler returns successfully, an audit `USER_ACTION` with `ALLOW` result is emitted. If the handler fails, an audit `SYSTEM_ERROR` with `FAILURE` result is emitted.

11. **Agent Completion**: After the entry function returns, the agent transitions to `COMPLETED` state and an `AGENT_COMPLETED` audit event with `SUCCESS` result is emitted.

12. **Audit Dump**: The complete audit log can be displayed to the VGA console in chronological order (oldest to newest) via `audit_dump_to_console()`, showing all events with their structured fields formatted for human readability.

## Prerequisites

- **macOS** (Apple Silicon or Intel)
- **clang** - C compiler (comes with Xcode Command Line Tools)
- **lld** - Linker (comes with clang)
- **i686-elf-grub** - GRUB for cross-compilation (install via Homebrew: `brew install i686-elf-grub`)
- **qemu-system-i386** - QEMU emulator (install via Homebrew: `brew install qemu`)
- **xorriso** - ISO creation tool (install via Homebrew: `brew install xorriso`)

## Quick Start

### Build and Run

```bash
make kernel    # Build kernel ELF (build/kernel.elf)
make iso       # Generate bootable ISO (build/agentos.iso)
make run       # Boot in QEMU
```

When you run `make run`, the kernel will:
1. Initialize all subsystems (audit, capability, intent router, agent)
2. Create two agents ("init" and "demo")
3. Grant `CAP_CONSOLE_WRITE` to the "init" agent only
4. Run both agents (both attempt to submit console write intents)
5. Display the complete audit log to the VGA console showing all events including capability grants, intent submissions, allow/deny decisions, and agent lifecycle events
6. Halt the system

### Debugging

```bash
make debug     # Start QEMU in debug mode (paused, waiting for GDB)
# In another terminal:
gdb build/kernel.elf -x tools/gdb-init
```

See [docs/dev-setup.md](docs/dev-setup.md) for detailed debugging instructions.

## Make Targets

- `make kernel` - Build the kernel ELF (`build/kernel.elf`)
- `make iso` - Build kernel and generate bootable ISO (`build/agentos.iso`)
- `make run` - Build ISO and boot in QEMU
- `make debug` - Build ISO and start QEMU in debug mode (GDB server on port 1234)
- `make clean` - Remove all build artifacts

## Project Structure

```
agentos/
├── boot/
│   ├── grub/
│   │   └── grub.cfg          # GRUB bootloader configuration
│   └── linker.ld             # Kernel linker script
├── build/                    # Build artifacts (git-ignored)
│   ├── kernel.elf            # Compiled kernel
│   └── agentos.iso          # Bootable ISO image
├── docs/                     # Documentation
│   ├── dev-setup.md          # Development setup and debugging
│   ├── philosophy.md         # Project philosophy
│   └── roadmap.md            # Development roadmap
├── kernel/                   # Kernel source code
│   ├── agent/                # Agent lifecycle management
│   │   ├── agent.c
│   │   └── agent.h
│   ├── arch/
│   │   └── x86_64/
│   │       └── entry.S       # Multiboot2 entry point (i386 assembly)
│   ├── audit/                # Structured audit logging
│   │   ├── audit.c
│   │   └── audit.h
│   ├── cap/                  # Capability-based security
│   │   ├── cap.c
│   │   └── cap.h
│   ├── intent/               # Intent-based execution
│   │   ├── handlers.c        # Intent handler implementations
│   │   ├── handlers.h
│   │   ├── intent.h          # Intent structure and capability mapping
│   │   ├── router.c          # Intent handler registry
│   │   └── router.h
│   ├── main.c                # Kernel main entry point
│   ├── syscall/              # System call interface with capability enforcement
│   │   ├── syscall.c
│   │   └── syscall.h
│   ├── vga.c                 # VGA text-mode driver
│   └── vga.h
├── tools/                    # Development tools
│   └── gdb-init              # GDB initialization script
└── Makefile                  # Build system
```

## Technical Details

- **Architecture**: i386 (32-bit)
- **Boot Protocol**: Multiboot2
- **Bootloader**: GRUB
- **Toolchain**: clang + lld
- **Target**: freestanding (no libc)
- **Emulation**: QEMU i386

## Documentation

- [Development Setup](docs/dev-setup.md) - Setup, building, and debugging
- [Philosophy](docs/philosophy.md) - Project design principles
- [Roadmap](docs/roadmap.md) - Development roadmap

## License

See [LICENSE](LICENSE) file for details.
