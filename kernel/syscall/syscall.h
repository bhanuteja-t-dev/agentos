// AgentOS System Call Module
// Week 2 Day 1: Capability-enforced system calls

#ifndef SYSCALL_H
#define SYSCALL_H

#include "audit/audit.h"  // For agent_id_t
#include "intent/intent.h"  // For intent_t

// System call: Write to console
// Enforces CAP_CONSOLE_WRITE capability
// Returns: 0 on success, -1 on failure (capability denied or invalid args)
int sys_console_write(agent_id_t agent_id, const char* msg);

// System call: Submit an intent for execution
// Validates intent action, checks required capabilities, and executes intent
// Returns: 0 on success, -1 on failure (invalid args, capability denied, or execution error)
int sys_intent_submit(agent_id_t agent_id, const intent_t* intent);

#endif // SYSCALL_H
