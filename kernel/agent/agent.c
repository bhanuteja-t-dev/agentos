// AgentOS Agent Module Implementation
// Week 2 Day 1: Fixed-size agent table

#include "agent.h"
#include "audit/audit.h"

// Fixed-size agent table
static agent_t agent_table[AGENT_MAX_COUNT];

// Number of agents currently in the table
static unsigned int agent_count_value = 0;

// Initialization flag
static int agent_initialized = 0;

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

// Helper function to build audit message: "agent_name suffix"
static void build_audit_msg(char* msg, const char* agent_name, const char* suffix, unsigned int max_len) {
    unsigned int pos = 0;
    
    // Copy agent name
    unsigned int name_len = str_len(agent_name);
    for (unsigned int i = 0; i < name_len && pos < max_len - 1; i++) {
        msg[pos++] = agent_name[i];
    }
    
    // Append space if there's room
    if (pos < max_len - 1) {
        msg[pos++] = ' ';
    }
    
    // Append suffix
    unsigned int suffix_len = str_len(suffix);
    for (unsigned int i = 0; i < suffix_len && pos < max_len - 1; i++) {
        msg[pos++] = suffix[i];
    }
    
    msg[pos] = '\0';
}

void agent_init(void) {
    // Initialize all agent slots to invalid state
    for (unsigned int i = 0; i < AGENT_MAX_COUNT; i++) {
        agent_table[i].name[0] = '\0';
        agent_table[i].entry = 0;
        agent_table[i].context = 0;
        agent_table[i].state = AGENT_STATE_INVALID;
    }
    
    agent_count_value = 0;
    agent_initialized = 1;
}

int agent_create(const char* name, agent_entry_t entry, void* context) {
    // Check if initialized
    if (!agent_initialized) {
        return -1;
    }
    
    // Validate arguments
    if (name == 0 || entry == 0) {
        return -1;
    }
    
    // Check name length (must fit in buffer, leave room for null terminator)
    if (str_len(name) >= AGENT_NAME_MAX) {
        return -1;
    }
    
    // Check if table is full
    if (agent_count_value >= AGENT_MAX_COUNT) {
        return -1;
    }
    
    // Find first available slot
    for (unsigned int i = 0; i < AGENT_MAX_COUNT; i++) {
        if (agent_table[i].state == AGENT_STATE_INVALID) {
            // Copy name
            str_copy(agent_table[i].name, name, AGENT_NAME_MAX);
            
            // Set entry point and context
            agent_table[i].entry = entry;
            agent_table[i].context = context;
            
            // Set state to created
            agent_table[i].state = AGENT_STATE_CREATED;
            
            // Increment count
            agent_count_value++;
            
            // Emit audit event for agent creation
            char audit_msg[128];
            build_audit_msg(audit_msg, name, "agent created", 128);
            audit_emit(AUDIT_TYPE_AGENT_CREATED, (int)i, audit_msg);
            
            // Return agent ID (array index)
            return (int)i;
        }
    }
    
    // Should not reach here if table has space
    return -1;
}

int agent_run(int id) {
    // Check if initialized
    if (!agent_initialized) {
        return -1;
    }
    
    // Validate agent ID
    if (id < 0 || id >= AGENT_MAX_COUNT) {
        return -1;
    }
    
    // Check if agent slot is valid and in CREATED state
    agent_t* agent = &agent_table[id];
    if (agent->state != AGENT_STATE_CREATED) {
        return -1;
    }
    
    // Validate entry point
    if (agent->entry == 0) {
        return -1;
    }
    
    // Update state to running
    agent->state = AGENT_STATE_RUNNING;
    
    // Emit audit event for agent started
    char audit_msg[128];
    build_audit_msg(audit_msg, agent->name, "agent started", 128);
    audit_emit(AUDIT_TYPE_AGENT_STARTED, id, audit_msg);
    
    // Call agent entry point with context
    agent->entry(agent->context);
    
    // Update state to completed
    agent->state = AGENT_STATE_COMPLETED;
    
    // Emit audit event for agent completed
    build_audit_msg(audit_msg, agent->name, "agent completed", 128);
    audit_emit(AUDIT_TYPE_AGENT_COMPLETED, id, audit_msg);
    
    return 0;
}

unsigned int agent_count(void) {
    return agent_count_value;
}
