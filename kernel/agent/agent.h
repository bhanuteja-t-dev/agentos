// AgentOS Agent Module
// Week 2 Day 1: Fixed-size agent table

#ifndef AGENT_H
#define AGENT_H

// Maximum number of agents
#define AGENT_MAX_COUNT 16

// Maximum agent name length (including null terminator)
#define AGENT_NAME_MAX 64

// Agent state
typedef enum {
    AGENT_STATE_INVALID = 0,  // Unused slot
    AGENT_STATE_CREATED,       // Created but not run yet
    AGENT_STATE_RUNNING,       // Currently running
    AGENT_STATE_COMPLETED      // Execution completed
} agent_state_t;

// Agent entry function type: void entry(void* context)
typedef void (*agent_entry_t)(void* context);

// Agent structure
typedef struct {
    char name[AGENT_NAME_MAX];      // Agent name (null-terminated)
    agent_entry_t entry;             // Entry point function
    void* context;                   // Context pointer passed to entry
    agent_state_t state;             // Current state
} agent_t;

// Initialize the agent system
void agent_init(void);

// Create a new agent
// Returns: agent ID (0-15) on success, -1 on failure (table full or invalid args)
int agent_create(const char* name, agent_entry_t entry, void* context);

// Run an agent by ID
// Returns: 0 on success, -1 on failure (invalid ID or agent not in CREATED state)
int agent_run(int id);

// Get the number of created agents
unsigned int agent_count(void);

#endif // AGENT_H
