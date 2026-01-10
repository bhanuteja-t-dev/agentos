// AgentOS Kernel Main Entry Point
// Week 2 Day 1: Agent and audit system integration

#include "agent/agent.h"
#include "audit/audit.h"

// Simple entry function for "init" agent
static void init_agent_entry(void* context) {
    (void)context;  // Unused parameter
    // Agent runs and completes
}

// Simple entry function for "demo" agent
static void demo_agent_entry(void* context) {
    (void)context;  // Unused parameter
    // Agent runs and completes
}

void kernel_main(void) {
    // Initialize audit system first (it emits its own init event)
    audit_init();
    
    // Emit boot event
    audit_emit(AUDIT_TYPE_SYSTEM_INIT, -1, "BOOT: Kernel starting");
    
    // Initialize agent system
    agent_init();
    
    // Create "init" agent
    int init_id = agent_create("init", init_agent_entry, (void*)0);
    if (init_id >= 0) {
        // Run the agent (agent module will emit audit events)
        if (agent_run(init_id) != 0) {
            audit_emit(AUDIT_TYPE_AGENT_ERROR, init_id, "init agent failed to run");
        }
    } else {
        audit_emit(AUDIT_TYPE_SYSTEM_ERROR, -1, "Failed to create init agent");
    }
    
    // Create "demo" agent
    int demo_id = agent_create("demo", demo_agent_entry, (void*)0);
    if (demo_id >= 0) {
        // Run the agent (agent module will emit audit events)
        if (agent_run(demo_id) != 0) {
            audit_emit(AUDIT_TYPE_AGENT_ERROR, demo_id, "demo agent failed to run");
        }
    } else {
        audit_emit(AUDIT_TYPE_SYSTEM_ERROR, -1, "Failed to create demo agent");
    }
    
    // Dump audit log to VGA console (all events in chronological order)
    audit_dump_to_console();
    
    // Halt the CPU in infinite loop
    while (1) {
        __asm__ volatile ("hlt");
    }
}
