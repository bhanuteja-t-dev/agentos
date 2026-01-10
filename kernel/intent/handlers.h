// AgentOS Intent Handler Declarations
// Week 2 Day 1: Concrete intent handlers

#ifndef INTENT_HANDLERS_H
#define INTENT_HANDLERS_H

#include "intent.h"

// Handler for INTENT_CONSOLE_WRITE intent
// Prints the intent payload to VGA console
// Parameters: agent_id (unused but required by handler signature), intent (contains payload to print)
// Returns: 0 on success, -1 on failure
int handle_console_write(int agent_id, const intent_t* intent);

#endif // INTENT_HANDLERS_H
