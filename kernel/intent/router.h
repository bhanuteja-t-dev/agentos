// AgentOS Intent Router Module
// Week 2 Day 1: Intent handler registry for dynamic intent dispatch

#ifndef INTENT_ROUTER_H
#define INTENT_ROUTER_H

#include "intent.h"  // For intent_action_t and intent_t

// Intent handler function type
// Parameters: agent_id (the agent submitting the intent), intent (pointer to intent structure)
// Returns: 0 on success, -1 on failure
typedef int (*intent_handler_t)(int agent_id, const intent_t* intent);

// Initialize the intent router system
void intent_router_init(void);

// Register a handler for a specific intent action
// Returns: 0 on success, -1 on failure (invalid action or action already registered)
int intent_register_handler(intent_action_t action, intent_handler_t handler);

// Get the handler for a specific intent action
// Returns: handler function pointer on success, 0 (NULL) if no handler registered or invalid action
intent_handler_t intent_get_handler(intent_action_t action);

#endif // INTENT_ROUTER_H
