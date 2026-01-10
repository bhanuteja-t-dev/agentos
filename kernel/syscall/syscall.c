// AgentOS System Call Module Implementation
// Week 2 Day 1: Capability-enforced system calls

#include "syscall.h"
#include "cap/cap.h"
#include "vga.h"
#include "audit/audit.h"
#include "intent/intent.h"
#include "intent/router.h"

// Helper function to get string length (no libc)
static unsigned int str_len(const char* s) {
    unsigned int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

// Convert integer to string (no libc, for agent_id)
static void int_to_string(int value, char* buffer, unsigned int buffer_size) {
    if (buffer_size == 0) {
        return;
    }
    
    unsigned int pos = 0;
    unsigned int abs_value;
    
    // Handle negative sign
    if (value < 0) {
        if (pos >= buffer_size - 1) {
            buffer[0] = '\0';
            return;
        }
        buffer[pos++] = '-';
        // Convert to unsigned for arithmetic (avoid overflow issues)
        if (value == -2147483647 - 1) {
            // Handle INT_MIN specially
            abs_value = 2147483648U;
        } else {
            abs_value = (unsigned int)(-value);
        }
    } else {
        abs_value = (unsigned int)value;
    }
    
    // Convert to string (reverse order first)
    char temp[16];
    unsigned int temp_pos = 0;
    if (abs_value == 0) {
        temp[temp_pos++] = '0';
    } else {
        while (abs_value > 0 && temp_pos < 15) {
            temp[temp_pos++] = '0' + (abs_value % 10);
            abs_value /= 10;
        }
    }
    
    // Reverse and copy to buffer
    while (temp_pos > 0 && pos < buffer_size - 1) {
        buffer[pos++] = temp[--temp_pos];
    }
    buffer[pos] = '\0';
}

int sys_console_write(agent_id_t agent_id, const char* msg) {
    // Validate arguments
    if (msg == 0) {
        return -1;
    }
    
    // Validate agent ID
    if (agent_id < 0 || agent_id >= AGENT_MAX_COUNT) {
        return -1;
    }
    
    // Check capability: agent must have CAP_CONSOLE_WRITE
    if (!cap_has(agent_id, CAP_CONSOLE_WRITE)) {
        // Capability denied - emit audit DENY event
        char audit_msg[128];
        unsigned int pos = 0;
        
        // Build message: "DENY console_write agent ID: message"
        const char* deny_str = "DENY console_write agent ";
        unsigned int deny_len = str_len(deny_str);
        for (unsigned int i = 0; i < deny_len && pos < 127; i++) {
            audit_msg[pos++] = deny_str[i];
        }
        
        // Add agent ID
        char id_str[16];
        int_to_string(agent_id, id_str, 16);
        unsigned int id_len = str_len(id_str);
        for (unsigned int i = 0; i < id_len && pos < 127; i++) {
            audit_msg[pos++] = id_str[i];
        }
        
        // Add separator
        if (pos < 127) {
            audit_msg[pos++] = ':';
            audit_msg[pos++] = ' ';
        }
        
        // Copy message (truncate if too long)
        unsigned int msg_len = str_len(msg);
        unsigned int max_msg_len = 127 - pos;
        if (msg_len > max_msg_len) {
            msg_len = max_msg_len;
        }
        for (unsigned int i = 0; i < msg_len && pos < 127; i++) {
            audit_msg[pos++] = msg[i];
        }
        
        audit_msg[pos] = '\0';
        
        audit_emit(AUDIT_TYPE_SYSTEM_ERROR, agent_id, audit_msg);
        
        return -1;
    }
    
    // Capability allowed - write to console
    vga_write(msg);
    
    // Emit audit ALLOW event
    char audit_msg[128];
    unsigned int pos = 0;
    
    // Build message: "ALLOW console_write agent ID: message"
    const char* allow_str = "ALLOW console_write agent ";
    unsigned int allow_len = str_len(allow_str);
    for (unsigned int i = 0; i < allow_len && pos < 127; i++) {
        audit_msg[pos++] = allow_str[i];
    }
    
    // Add agent ID
    char id_str[16];
    int_to_string(agent_id, id_str, 16);
    unsigned int id_len = str_len(id_str);
    for (unsigned int i = 0; i < id_len && pos < 127; i++) {
        audit_msg[pos++] = id_str[i];
    }
    
    // Add separator
    if (pos < 127) {
        audit_msg[pos++] = ':';
        audit_msg[pos++] = ' ';
    }
    
    // Copy message (truncate if too long)
    unsigned int msg_len = str_len(msg);
    unsigned int max_msg_len = 127 - pos;
    if (msg_len > max_msg_len) {
        msg_len = max_msg_len;
    }
    for (unsigned int i = 0; i < msg_len && pos < 127; i++) {
        audit_msg[pos++] = msg[i];
    }
    
    audit_msg[pos] = '\0';
    
    audit_emit(AUDIT_TYPE_USER_ACTION, agent_id, audit_msg);
    
    return 0;
}

// Helper function to convert intent action to string (for audit)
static const char* intent_action_to_string(intent_action_t action) {
    switch (action) {
        case INTENT_CONSOLE_WRITE:
            return "CONSOLE_WRITE";
        default:
            return "UNKNOWN";
    }
}

int sys_intent_submit(agent_id_t agent_id, const intent_t* intent) {
    // Validate arguments
    if (intent == 0) {
        return -1;
    }
    
    // Validate agent ID
    if (agent_id < 0 || agent_id >= AGENT_MAX_COUNT) {
        return -1;
    }
    
    // Validate intent action
    if (intent->action >= INTENT_MAX) {
        return -1;
    }
    
    // Audit INTENT_SUBMIT event
    char audit_msg[128];
    unsigned int pos = 0;
    
    // Build message: "INTENT_SUBMIT action agent ID"
    const char* submit_str = "INTENT_SUBMIT ";
    unsigned int submit_len = str_len(submit_str);
    for (unsigned int i = 0; i < submit_len && pos < 127; i++) {
        audit_msg[pos++] = submit_str[i];
    }
    
    // Add action string
    const char* action_str = intent_action_to_string(intent->action);
    unsigned int action_len = str_len(action_str);
    for (unsigned int i = 0; i < action_len && pos < 127; i++) {
        audit_msg[pos++] = action_str[i];
    }
    
    // Add " agent "
    const char* agent_str = " agent ";
    unsigned int agent_len = str_len(agent_str);
    for (unsigned int i = 0; i < agent_len && pos < 127; i++) {
        audit_msg[pos++] = agent_str[i];
    }
    
    // Add agent ID
    char id_str[16];
    int_to_string(agent_id, id_str, 16);
    unsigned int id_len = str_len(id_str);
    for (unsigned int i = 0; i < id_len && pos < 127; i++) {
        audit_msg[pos++] = id_str[i];
    }
    
    audit_msg[pos] = '\0';
    audit_emit(AUDIT_TYPE_USER_ACTION, agent_id, audit_msg);
    
    // Lookup handler for this intent action
    intent_handler_t handler = intent_get_handler(intent->action);
    
    if (handler == 0) {
        // No handler registered for this action - emit audit error
        pos = 0;
        
        const char* error_str = "No handler registered for intent ";
        unsigned int error_len = str_len(error_str);
        for (unsigned int i = 0; i < error_len && pos < 127; i++) {
            audit_msg[pos++] = error_str[i];
        }
        
        action_str = intent_action_to_string(intent->action);
        action_len = str_len(action_str);
        for (unsigned int i = 0; i < action_len && pos < 127; i++) {
            audit_msg[pos++] = action_str[i];
        }
        
        audit_msg[pos] = '\0';
        audit_emit(AUDIT_TYPE_SYSTEM_ERROR, agent_id, audit_msg);
        return -1;
    }
    
    // Map action to required capability
    cap_mask_t required_cap = intent_action_to_capability(intent->action);
    
    // Enforce capability check
    if (!cap_has(agent_id, required_cap)) {
        // Capability denied - emit audit DENY event
        pos = 0;
        
        // Build message: "DENY intent action agent ID: payload"
        const char* deny_str = "DENY intent ";
        unsigned int deny_len = str_len(deny_str);
        for (unsigned int i = 0; i < deny_len && pos < 127; i++) {
            audit_msg[pos++] = deny_str[i];
        }
        
        // Add action string
        action_str = intent_action_to_string(intent->action);
        action_len = str_len(action_str);
        for (unsigned int i = 0; i < action_len && pos < 127; i++) {
            audit_msg[pos++] = action_str[i];
        }
        
        // Add " agent "
        agent_str = " agent ";
        agent_len = str_len(agent_str);
        for (unsigned int i = 0; i < agent_len && pos < 127; i++) {
            audit_msg[pos++] = agent_str[i];
        }
        
        // Add agent ID
        int_to_string(agent_id, id_str, 16);
        id_len = str_len(id_str);
        for (unsigned int i = 0; i < id_len && pos < 127; i++) {
            audit_msg[pos++] = id_str[i];
        }
        
        // Add separator
        if (pos < 127) {
            audit_msg[pos++] = ':';
            audit_msg[pos++] = ' ';
        }
        
        // Copy payload (truncate if too long)
        unsigned int payload_len = str_len(intent->payload);
        unsigned int max_payload_len = 127 - pos;
        if (payload_len > max_payload_len) {
            payload_len = max_payload_len;
        }
        for (unsigned int i = 0; i < payload_len && pos < 127; i++) {
            audit_msg[pos++] = intent->payload[i];
        }
        
        audit_msg[pos] = '\0';
        audit_emit(AUDIT_TYPE_SYSTEM_ERROR, agent_id, audit_msg);
        
        return -1;
    }
    
    // Capability allowed - call handler
    int handler_result = handler(agent_id, intent);
    
    if (handler_result != 0) {
        // Handler execution failed - emit audit failure event
        pos = 0;
        
        const char* error_str = "Handler execution failed for intent ";
        unsigned int error_len = str_len(error_str);
        for (unsigned int i = 0; i < error_len && pos < 127; i++) {
            audit_msg[pos++] = error_str[i];
        }
        
        action_str = intent_action_to_string(intent->action);
        action_len = str_len(action_str);
        for (unsigned int i = 0; i < action_len && pos < 127; i++) {
            audit_msg[pos++] = action_str[i];
        }
        
        audit_msg[pos] = '\0';
        audit_emit(AUDIT_TYPE_SYSTEM_ERROR, agent_id, audit_msg);
        return -1;
    }
    
    // Handler executed successfully - emit audit ALLOW event
    pos = 0;
    
    // Build message: "ALLOW intent action agent ID: payload"
    const char* allow_str = "ALLOW intent ";
    unsigned int allow_len = str_len(allow_str);
    for (unsigned int i = 0; i < allow_len && pos < 127; i++) {
        audit_msg[pos++] = allow_str[i];
    }
    
    // Add action string
    action_str = intent_action_to_string(intent->action);
    action_len = str_len(action_str);
    for (unsigned int i = 0; i < action_len && pos < 127; i++) {
        audit_msg[pos++] = action_str[i];
    }
    
    // Add " agent "
    agent_str = " agent ";
    agent_len = str_len(agent_str);
    for (unsigned int i = 0; i < agent_len && pos < 127; i++) {
        audit_msg[pos++] = agent_str[i];
    }
    
    // Add agent ID
    int_to_string(agent_id, id_str, 16);
    id_len = str_len(id_str);
    for (unsigned int i = 0; i < id_len && pos < 127; i++) {
        audit_msg[pos++] = id_str[i];
    }
    
    // Add separator
    if (pos < 127) {
        audit_msg[pos++] = ':';
        audit_msg[pos++] = ' ';
    }
    
    // Copy payload (truncate if too long)
    unsigned int payload_len = str_len(intent->payload);
    unsigned int max_payload_len = 127 - pos;
    if (payload_len > max_payload_len) {
        payload_len = max_payload_len;
    }
    for (unsigned int i = 0; i < payload_len && pos < 127; i++) {
        audit_msg[pos++] = intent->payload[i];
    }
    
    audit_msg[pos] = '\0';
    audit_emit(AUDIT_TYPE_USER_ACTION, agent_id, audit_msg);
    
    return 0;
}
