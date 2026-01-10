// AgentOS System Call Module
// Week 2 Day 1: Capability-enforced system calls

#ifndef SYSCALL_H
#define SYSCALL_H

#include "audit/audit.h"  // For agent_id_t

// System call: Write to console
// Enforces CAP_CONSOLE_WRITE capability
// Returns: 0 on success, -1 on failure (capability denied or invalid args)
int sys_console_write(agent_id_t agent_id, const char* msg);

#endif // SYSCALL_H
