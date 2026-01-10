// AgentOS Kernel Main Entry Point
// Week 2 Day 1: Agent and audit system integration with capability enforcement

#include "agent/agent.h"
#include "audit/audit.h"
#include "cap/cap.h"
#include "syscall/syscall.h"

// Simple entry function for "init" agent
static void init_agent_entry(void* context) {
    // Context is the agent ID
    int agent_id = (int)(long)context;
    
    // Attempt to write to console (should succeed if capability granted)
    sys_console_write(agent_id, "init agent: Hello from init!\n");
}

// Simple entry function for "demo" agent
static void demo_agent_entry(void* context) {
    // Context is the agent ID
    int agent_id = (int)(long)context;
    
    // Attempt to write to console (should fail if capability not granted)
    sys_console_write(agent_id, "demo agent: Hello from demo!\n");
}

void kernel_main(void) {
    // Initialize audit system first (it emits its own init event)
    audit_init();
    
    // Emit boot event
    audit_emit(AUDIT_TYPE_SYSTEM_INIT, -1, "BOOT: Kernel starting");
    
    // Initialize capability system
    cap_init();
    
    // Initialize agent system
    agent_init();
    
    // Create "init" agent (will be agent 0, assuming sequential creation)
    int init_id = agent_create("init", init_agent_entry, (void*)(long)0);
    if (init_id < 0) {
        audit_emit(AUDIT_TYPE_SYSTEM_ERROR, -1, "Failed to create init agent");
        audit_dump_to_console();
        while (1) {
            __asm__ volatile ("hlt");
        }
    }
    // Note: Since agent_create returns the first available slot index,
    // init_id should be 0 (first agent created). Context was set to 0 above.
    
    // Create "demo" agent (will be agent 1, assuming sequential creation)
    int demo_id = agent_create("demo", demo_agent_entry, (void*)(long)1);
    if (demo_id < 0) {
        audit_emit(AUDIT_TYPE_SYSTEM_ERROR, -1, "Failed to create demo agent");
        audit_dump_to_console();
        while (1) {
            __asm__ volatile ("hlt");
        }
    }
    // Note: demo_id should be 1 (second agent created). Context was set to 1 above.
    
    // Grant CAP_CONSOLE_WRITE to init agent only
    if (cap_grant(init_id, CAP_CONSOLE_WRITE) != 0) {
        audit_emit(AUDIT_TYPE_SYSTEM_ERROR, -1, "Failed to grant capability to init agent");
    }
    
    // Run init agent (has capability, sys_console_write should succeed)
    // init_agent_entry will be called with context=0, which is init_id
    if (agent_run(init_id) != 0) {
        audit_emit(AUDIT_TYPE_AGENT_ERROR, init_id, "init agent failed to run");
    }
    
    // Run demo agent (no capability, sys_console_write should fail)
    // demo_agent_entry will be called with context=1, which is demo_id
    if (agent_run(demo_id) != 0) {
        audit_emit(AUDIT_TYPE_AGENT_ERROR, demo_id, "demo agent failed to run");
    }
    
    // Dump audit log to VGA console (all events in chronological order)
    audit_dump_to_console();
    
    // Halt the CPU in infinite loop
    while (1) {
        __asm__ volatile ("hlt");
    }
}
