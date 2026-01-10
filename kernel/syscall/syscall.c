// AgentOS System Call Module Implementation
// Week 2 Day 1: Capability-enforced system calls

#include "syscall.h"
#include "cap/cap.h"
#include "vga.h"
#include "audit/audit.h"
#include "intent/intent.h"
#include "intent/router.h"

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
        // Capability denied - emit audit DENY event with structured record
        // Structured fields: type=SYSTEM_ERROR, result=DENY, agent_id, intent_action=-1 (not intent-based)
        audit_emit(AUDIT_TYPE_SYSTEM_ERROR, AUDIT_RESULT_DENY, agent_id, -1, msg);
        
        return -1;
    }
    
    // Capability allowed - write to console
    vga_write(msg);
    
    // Emit audit ALLOW event with structured record
    // Structured fields: type=USER_ACTION, result=ALLOW, agent_id, intent_action=-1 (not intent-based)
    audit_emit(AUDIT_TYPE_USER_ACTION, AUDIT_RESULT_ALLOW, agent_id, -1, msg);
    
    return 0;
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
    
    // Audit INTENT_SUBMIT event with structured record
    // Structured fields: type=INTENT_SUBMIT, result=NONE, agent_id, intent_action
    // Message provides payload context (already captured in structured fields above)
    audit_emit(AUDIT_TYPE_INTENT_SUBMIT, AUDIT_RESULT_NONE, agent_id, (int)intent->action, intent->payload);
    
    // Lookup handler for this intent action
    intent_handler_t handler = intent_get_handler(intent->action);
    
    if (handler == 0) {
        // No handler registered for this action - emit audit error with structured record
        // Structured fields: type=SYSTEM_ERROR, result=FAILURE, agent_id, intent_action
        audit_emit(AUDIT_TYPE_SYSTEM_ERROR, AUDIT_RESULT_FAILURE, agent_id, (int)intent->action, "No handler registered for intent action");
        return -1;
    }
    
    // Map action to required capability
    cap_mask_t required_cap = intent_action_to_capability(intent->action);
    
    // Enforce capability check
    if (!cap_has(agent_id, required_cap)) {
        // Capability denied - emit audit DENY event with structured record
        // Structured fields: type=SYSTEM_ERROR, result=DENY, agent_id, intent_action
        // Message provides payload context (capability denial details already in structured fields)
        audit_emit(AUDIT_TYPE_SYSTEM_ERROR, AUDIT_RESULT_DENY, agent_id, (int)intent->action, intent->payload);
        
        return -1;
    }
    
    // Capability allowed - call handler
    int handler_result = handler(agent_id, intent);
    
    if (handler_result != 0) {
        // Handler execution failed - emit audit failure event with structured record
        // Structured fields: type=SYSTEM_ERROR, result=FAILURE, agent_id, intent_action
        // Message provides payload context (handler failure details)
        audit_emit(AUDIT_TYPE_SYSTEM_ERROR, AUDIT_RESULT_FAILURE, agent_id, (int)intent->action, intent->payload);
        return -1;
    }
    
    // Handler executed successfully - emit audit ALLOW event with structured record
    // Structured fields: type=USER_ACTION, result=ALLOW, agent_id, intent_action
    // Message provides payload context (intent execution details)
    audit_emit(AUDIT_TYPE_USER_ACTION, AUDIT_RESULT_ALLOW, agent_id, (int)intent->action, intent->payload);
    
    return 0;
}
