// AgentOS Intent Handler Implementations
// Week 2 Day 1: Concrete intent handlers

#include "handlers.h"
#include "vga.h"

// Handler for INTENT_CONSOLE_WRITE intent
// Prints the intent payload to VGA console
// Parameters: agent_id (unused but required by handler signature), intent (contains payload to print)
// Returns: 0 on success, -1 on failure
int handle_console_write(int agent_id, const intent_t* intent) {
    // Mark unused parameter to suppress warning
    (void)agent_id;
    
    // Validate intent pointer
    if (intent == 0) {
        return -1;
    }
    
    // Print payload to VGA console
    vga_write(intent->payload);
    
    return 0;
}
