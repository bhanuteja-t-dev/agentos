// AgentOS Intent Router Module Implementation
// Week 2 Day 1: Intent handler registry for dynamic intent dispatch

#include "router.h"

// Fixed-size handler registry table (one entry per intent action type)
static intent_handler_t handler_table[INTENT_MAX];

// Initialization flag
static int router_initialized = 0;

void intent_router_init(void) {
    // Clear all handler entries (set to 0/NULL)
    for (unsigned int i = 0; i < INTENT_MAX; i++) {
        handler_table[i] = 0;
    }
    
    router_initialized = 1;
}

int intent_register_handler(intent_action_t action, intent_handler_t handler) {
    // Check if initialized
    if (!router_initialized) {
        return -1;
    }
    
    // Validate action
    if (action >= INTENT_MAX) {
        return -1;
    }
    
    // Validate handler pointer
    if (handler == 0) {
        return -1;
    }
    
    // Check if handler already registered for this action
    if (handler_table[action] != 0) {
        return -1;  // Action already has a handler
    }
    
    // Register handler
    handler_table[action] = handler;
    
    return 0;
}

intent_handler_t intent_get_handler(intent_action_t action) {
    // Check if initialized
    if (!router_initialized) {
        return 0;
    }
    
    // Validate action
    if (action >= INTENT_MAX) {
        return 0;
    }
    
    // Return handler (may be 0/NULL if not registered)
    return handler_table[action];
}
