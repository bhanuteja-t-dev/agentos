// AgentOS System Call Module Implementation
// Week 2 Day 1: Capability-enforced system calls

#include "syscall.h"
#include "cap/cap.h"
#include "vga.h"
#include "audit/audit.h"

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
