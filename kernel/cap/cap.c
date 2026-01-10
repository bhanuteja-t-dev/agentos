// AgentOS Capability Module Implementation
// Week 2 Day 1: Per-agent capability bitmask

#include "cap.h"
#include "audit/audit.h"

// Capability bitmask for each agent (fixed-size array, no heap)
static cap_mask_t agent_caps[AGENT_MAX_COUNT];

// Initialization flag
static int cap_initialized = 0;

// Helper function to get string length (no libc)
static unsigned int str_len(const char* s) {
    unsigned int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

// Convert capability mask to string representation (for audit)
static void cap_mask_to_string(cap_mask_t mask, char* buffer, unsigned int buffer_size) {
    if (buffer_size == 0) {
        return;
    }
    
    unsigned int pos = 0;
    
    if (mask == CAP_NONE) {
        // Copy "NONE"
        const char* none_str = "NONE";
        unsigned int none_len = str_len(none_str);
        for (unsigned int i = 0; i < none_len && pos < buffer_size - 1; i++) {
            buffer[pos++] = none_str[i];
        }
        buffer[pos] = '\0';
        return;
    }
    
    // Build capability string by checking each bit
    unsigned int first = 1;
    
    if (mask & CAP_CONSOLE_WRITE) {
        if (!first && pos < buffer_size - 1) {
            buffer[pos++] = '|';
        }
        first = 0;
        const char* console_str = "CONSOLE_WRITE";
        unsigned int console_len = str_len(console_str);
        for (unsigned int i = 0; i < console_len && pos < buffer_size - 1; i++) {
            buffer[pos++] = console_str[i];
        }
    }
    
    // Future capabilities can be added here
    // if (mask & CAP_SOME_OTHER) { ... }
    
    buffer[pos] = '\0';
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

void cap_init(void) {
    // Initialize all agent capabilities to CAP_NONE
    for (unsigned int i = 0; i < AGENT_MAX_COUNT; i++) {
        agent_caps[i] = CAP_NONE;
    }
    
    cap_initialized = 1;
    
    // Emit audit event for capability system initialization with structured record
    audit_emit(AUDIT_TYPE_SYSTEM_INIT, AUDIT_RESULT_NONE, -1, -1, "Capability system initialized");
}

int cap_grant(agent_id_t agent_id, cap_mask_t mask) {
    // Check if initialized
    if (!cap_initialized) {
        return -1;
    }
    
    // Validate agent ID
    if (agent_id < 0 || agent_id >= AGENT_MAX_COUNT) {
        return -1;
    }
    
    // Grant capabilities (OR with existing mask)
    agent_caps[agent_id] |= mask;
    
    // Emit audit event for capability grant
    char audit_msg[128];
    unsigned int pos = 0;
    
    // Build message: "Granted CAPS to agent ID"
    const char* granted_str = "Granted ";
    unsigned int granted_len = str_len(granted_str);
    for (unsigned int i = 0; i < granted_len && pos < 127; i++) {
        audit_msg[pos++] = granted_str[i];
    }
    
    // Add capability mask string
    char cap_str[64];
    cap_mask_to_string(mask, cap_str, 64);
    unsigned int cap_len = str_len(cap_str);
    for (unsigned int i = 0; i < cap_len && pos < 127; i++) {
        audit_msg[pos++] = cap_str[i];
    }
    
    // Add " to agent "
    const char* to_agent_str = " to agent ";
    unsigned int to_agent_len = str_len(to_agent_str);
    for (unsigned int i = 0; i < to_agent_len && pos < 127; i++) {
        audit_msg[pos++] = to_agent_str[i];
    }
    
    // Add agent ID
    char id_str[16];
    int_to_string(agent_id, id_str, 16);
    unsigned int id_len = str_len(id_str);
    for (unsigned int i = 0; i < id_len && pos < 127; i++) {
        audit_msg[pos++] = id_str[i];
    }
    
    audit_msg[pos] = '\0';
    
    // Emit capability grant event with structured record (SUCCESS result, no intent involved)
    audit_emit(AUDIT_TYPE_USER_ACTION, AUDIT_RESULT_SUCCESS, agent_id, -1, audit_msg);
    
    return 0;
}

int cap_has(agent_id_t agent_id, cap_mask_t mask) {
    // Check if initialized
    if (!cap_initialized) {
        return 0;
    }
    
    // Validate agent ID
    if (agent_id < 0 || agent_id >= AGENT_MAX_COUNT) {
        return 0;
    }
    
    // Check if agent has all required capabilities (all bits in mask must be set)
    // This checks: (agent_caps[agent_id] & mask) == mask
    return (agent_caps[agent_id] & mask) == mask;
}
