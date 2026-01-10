// AgentOS Capability Module
// Week 2 Day 1: Per-agent capability bitmask

#ifndef CAP_H
#define CAP_H

#include "agent/agent.h"  // For AGENT_MAX_COUNT
#include "audit/audit.h"  // For agent_id_t

// Capability flags (bitmask)
#define CAP_NONE           0x00000000
#define CAP_CONSOLE_WRITE  0x00000001
// Future capabilities can be added as powers of 2:
// #define CAP_SOME_OTHER    0x00000002
// #define CAP_ANOTHER       0x00000004

// Capability bitmask type
typedef unsigned int cap_mask_t;

// Initialize the capability system
void cap_init(void);

// Grant capabilities to an agent
// Returns: 0 on success, -1 on failure (invalid agent_id)
int cap_grant(agent_id_t agent_id, cap_mask_t mask);

// Check if an agent has the specified capabilities (all bits must be set)
// Returns: 1 if agent has all capabilities, 0 otherwise
int cap_has(agent_id_t agent_id, cap_mask_t mask);

#endif // CAP_H
