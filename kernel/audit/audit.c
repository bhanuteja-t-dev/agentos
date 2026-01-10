// AgentOS Audit Log Module Implementation
// Week 2 Day 1: Fixed-size ring buffer audit log with structured records

#include "audit.h"
#include "vga.h"
#include "intent/intent.h"  // For INTENT_MAX and intent action values

// Ring buffer for audit events
static audit_event_t audit_buffer[AUDIT_MAX_EVENTS];

// Current write position (next event goes here)
static unsigned int audit_write_pos = 0;

// Number of events written (used to calculate oldest event)
static unsigned int audit_total_count = 0;

// Initialization flag
static int audit_initialized = 0;

// Helper function to copy string (no libc)
static void str_copy(char* dest, const char* src, unsigned int max_len) {
    unsigned int i = 0;
    while (src[i] != '\0' && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// Helper function to get string length (no libc)
static unsigned int str_len(const char* s) {
    unsigned int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

// Convert audit type to string
static const char* audit_type_to_string(audit_type_t type) {
    switch (type) {
        case AUDIT_TYPE_AGENT_CREATED:
            return "AGENT_CREATED";
        case AUDIT_TYPE_AGENT_STARTED:
            return "AGENT_STARTED";
        case AUDIT_TYPE_AGENT_COMPLETED:
            return "AGENT_COMPLETED";
        case AUDIT_TYPE_AGENT_ERROR:
            return "AGENT_ERROR";
        case AUDIT_TYPE_SYSTEM_INIT:
            return "SYSTEM_INIT";
        case AUDIT_TYPE_SYSTEM_ERROR:
            return "SYSTEM_ERROR";
        case AUDIT_TYPE_USER_ACTION:
            return "USER_ACTION";
        case AUDIT_TYPE_INTENT_SUBMIT:
            return "INTENT_SUBMIT";
        default:
            return "UNKNOWN";
    }
}

// Convert audit result to string
static const char* audit_result_to_string(audit_result_t result) {
    switch (result) {
        case AUDIT_RESULT_NONE:
            return "";
        case AUDIT_RESULT_ALLOW:
            return "ALLOW";
        case AUDIT_RESULT_DENY:
            return "DENY";
        case AUDIT_RESULT_SUCCESS:
            return "SUCCESS";
        case AUDIT_RESULT_FAILURE:
            return "FAILURE";
        default:
            return "";
    }
}

// Convert intent action to string (for audit display)
static const char* intent_action_to_string_display(audit_intent_action_t action) {
    // Use values from intent.h (0 = INTENT_CONSOLE_WRITE, etc.)
    // -1 indicates not applicable
    if (action == -1) {
        return "";
    }
    switch (action) {
        case 0:  // INTENT_CONSOLE_WRITE
            return "CONSOLE_WRITE";
        default:
            return "UNKNOWN";
    }
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
        // For negative values, use two's complement representation
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

void audit_init(void) {
    // Initialize all event slots
    for (unsigned int i = 0; i < AUDIT_MAX_EVENTS; i++) {
        audit_buffer[i].type = AUDIT_TYPE_SYSTEM_INIT;
        audit_buffer[i].result = AUDIT_RESULT_NONE;
        audit_buffer[i].agent_id = -1;
        audit_buffer[i].intent_action = -1;  // -1 indicates not applicable
        audit_buffer[i].sequence = 0;
        audit_buffer[i].message[0] = '\0';
    }
    
    audit_write_pos = 0;
    audit_total_count = 0;
    audit_initialized = 1;
    
    // Emit initialization event with structured record
    audit_emit(AUDIT_TYPE_SYSTEM_INIT, AUDIT_RESULT_NONE, -1, -1, "Audit system initialized");
}

int audit_emit(audit_type_t type, audit_result_t result, agent_id_t agent_id, audit_intent_action_t intent_action, const char* message) {
    // Check if initialized
    if (!audit_initialized) {
        return -1;
    }
    
    // Validate arguments
    if (message == 0) {
        return -1;
    }
    
    // Validate type
    if (type >= AUDIT_TYPE_MAX) {
        return -1;
    }
    
    // Validate result
    if (result >= AUDIT_RESULT_MAX) {
        return -1;
    }
    
    // Check message length
    if (str_len(message) >= AUDIT_MSG_MAX) {
        return -1;
    }
    
    // Get current event slot
    audit_event_t* event = &audit_buffer[audit_write_pos];
    
    // Fill structured record
    event->type = type;
    event->result = result;
    event->agent_id = agent_id;
    event->intent_action = intent_action;
    event->sequence = audit_total_count;
    str_copy(event->message, message, AUDIT_MSG_MAX);
    
    // Advance write position (ring buffer: wrap around)
    audit_write_pos = (audit_write_pos + 1) % AUDIT_MAX_EVENTS;
    audit_total_count++;
    
    return 0;
}

void audit_dump_to_console(void) {
    // Clear screen and reset cursor to top-left
    vga_clear();
    
    if (!audit_initialized) {
        vga_write("Audit system not initialized\n");
        return;
    }
    
    // Check if we have any events
    if (audit_total_count == 0) {
        vga_write("No audit events to display\n");
        return;
    }
    
    // Calculate how many events to display
    unsigned int event_count = audit_total_count < AUDIT_MAX_EVENTS 
                               ? audit_total_count 
                               : AUDIT_MAX_EVENTS;
    
    // Determine the starting sequence number (oldest event to display)
    unsigned int start_seq = audit_total_count > AUDIT_MAX_EVENTS 
                             ? audit_total_count - AUDIT_MAX_EVENTS 
                             : 0;
    
    // Iterate through sequence numbers in chronological order (oldest to newest)
    for (unsigned int seq = start_seq; seq < start_seq + event_count; seq++) {
        // Calculate buffer position for this sequence number
        // When buffer is not full: event at seq is at buffer[seq]
        // When buffer is full: event at seq is at buffer[(write_pos + (seq - (count - MAX))) % MAX]
        // Simplified: for full buffer, oldest is at write_pos, next at (write_pos+1) % MAX, etc.
        unsigned int buffer_pos;
        if (audit_total_count <= AUDIT_MAX_EVENTS) {
            // Buffer not full: events stored sequentially starting at index 0
            buffer_pos = seq;
        } else {
            // Buffer full: events wrap around starting at write_pos
            // Oldest visible event is at write_pos (sequence count - MAX)
            // Position = (write_pos + (seq - start_seq)) % MAX
            buffer_pos = (audit_write_pos + (seq - start_seq)) % AUDIT_MAX_EVENTS;
        }
        
        audit_event_t* event = &audit_buffer[buffer_pos];
        
        // Verify this is the correct event (sanity check)
        if (event->sequence != seq) {
            // Skip if sequence doesn't match (shouldn't happen, but be safe)
            continue;
        }
        
        // Format structured event record into readable output (view layer - formatting on-the-fly)
        // Format: "[seq] TYPE agent:ID [result] [intent] message"
        {
            char display_msg[AUDIT_MSG_MAX + 80];  // Extra space for formatting structured fields
            unsigned int pos = 0;
            const char* type_str = audit_type_to_string(event->type);
            const char* result_str = audit_result_to_string(event->result);
            
            // Start with sequence number in brackets: "[seq] "
            display_msg[pos++] = '[';
            char seq_str[16];
            int_to_string((int)event->sequence, seq_str, 16);
            unsigned int seq_len = str_len(seq_str);
            for (unsigned int i = 0; i < seq_len && pos < AUDIT_MSG_MAX + 75; i++) {
                display_msg[pos++] = seq_str[i];
            }
            display_msg[pos++] = ']';
            display_msg[pos++] = ' ';
            
            // Add type string: "TYPE "
            unsigned int type_len = str_len(type_str);
            for (unsigned int i = 0; i < type_len && pos < AUDIT_MSG_MAX + 75; i++) {
                display_msg[pos++] = type_str[i];
            }
            display_msg[pos++] = ' ';
            
            // Add agent ID if valid: "agent:ID " or "system " if agent_id is -1
            if (event->agent_id >= 0) {
                // Format: "agent:ID "
                const char* agent_str = "agent:";
                unsigned int agent_len = str_len(agent_str);
                for (unsigned int i = 0; i < agent_len && pos < AUDIT_MSG_MAX + 75; i++) {
                    display_msg[pos++] = agent_str[i];
                }
                char id_str[16];
                int_to_string(event->agent_id, id_str, 16);
                unsigned int id_len = str_len(id_str);
                for (unsigned int i = 0; i < id_len && pos < AUDIT_MSG_MAX + 75; i++) {
                    display_msg[pos++] = id_str[i];
                }
                display_msg[pos++] = ' ';
            } else {
                // System event (agent_id is -1)
                const char* system_str = "system ";
                unsigned int system_len = str_len(system_str);
                for (unsigned int i = 0; i < system_len && pos < AUDIT_MSG_MAX + 75; i++) {
                    display_msg[pos++] = system_str[i];
                }
            }
            
            // Add result if not NONE: "[result] "
            unsigned int result_len = str_len(result_str);
            if (result_len > 0) {
                display_msg[pos++] = '[';
                for (unsigned int i = 0; i < result_len && pos < AUDIT_MSG_MAX + 75; i++) {
                    display_msg[pos++] = result_str[i];
                }
                display_msg[pos++] = ']';
                display_msg[pos++] = ' ';
            }
            
            // Add intent action if valid (>= 0): "[intent] "
            if (event->intent_action >= 0) {
                const char* intent_str = intent_action_to_string_display(event->intent_action);
                unsigned int intent_len = str_len(intent_str);
                if (intent_len > 0) {
                    display_msg[pos++] = '[';
                    for (unsigned int i = 0; i < intent_len && pos < AUDIT_MSG_MAX + 75; i++) {
                        display_msg[pos++] = intent_str[i];
                    }
                    display_msg[pos++] = ']';
                    display_msg[pos++] = ' ';
                }
            }
            
            // Add message (truncate if necessary)
            unsigned int msg_len = str_len(event->message);
            unsigned int max_msg_space = AUDIT_MSG_MAX + 75 - pos - 1;  // Leave room for newline
            if (msg_len > max_msg_space) {
                msg_len = max_msg_space;
            }
            for (unsigned int i = 0; i < msg_len && pos < AUDIT_MSG_MAX + 75; i++) {
                display_msg[pos++] = event->message[i];
            }
            
            // Add newline at end of each event
            display_msg[pos++] = '\n';
            display_msg[pos] = '\0';
            
            // Display using cursor-based write (handles '\n' automatically, scrolls if needed)
            vga_write(display_msg);
        }
    }
}
