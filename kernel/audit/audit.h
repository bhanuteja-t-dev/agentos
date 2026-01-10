// AgentOS Audit Log Module
// Week 2 Day 1: Fixed-size ring buffer audit log with structured records

#ifndef AUDIT_H
#define AUDIT_H

// Intent action type (using int to avoid circular dependency)
// Actual values come from intent.h (INTENT_CONSOLE_WRITE, INTENT_MAX, etc.)
// INTENT_MAX or -1 indicates "not applicable"
typedef int audit_intent_action_t;

// Maximum number of audit events in ring buffer
#define AUDIT_MAX_EVENTS 64

// Maximum audit message length (including null terminator)
#define AUDIT_MSG_MAX 128

// Agent ID type (matches agent module)
typedef int agent_id_t;

// Audit event type enum
typedef enum {
    AUDIT_TYPE_AGENT_CREATED = 0,
    AUDIT_TYPE_AGENT_STARTED,
    AUDIT_TYPE_AGENT_COMPLETED,
    AUDIT_TYPE_AGENT_ERROR,
    AUDIT_TYPE_SYSTEM_INIT,
    AUDIT_TYPE_SYSTEM_ERROR,
    AUDIT_TYPE_USER_ACTION,
    AUDIT_TYPE_INTENT_SUBMIT,
    AUDIT_TYPE_MAX               // Sentinel value
} audit_type_t;

// Audit result enum (for capability checks and execution results)
typedef enum {
    AUDIT_RESULT_NONE = 0,       // No result (e.g., info events)
    AUDIT_RESULT_ALLOW,          // Capability allowed / execution allowed
    AUDIT_RESULT_DENY,           // Capability denied
    AUDIT_RESULT_SUCCESS,        // Operation succeeded
    AUDIT_RESULT_FAILURE,        // Operation failed
    AUDIT_RESULT_MAX             // Sentinel value
} audit_result_t;

// Audit event structure (structured record)
typedef struct {
    audit_type_t type;           // Event type enum
    audit_result_t result;       // Result enum (ALLOW/DENY/SUCCESS/FAILURE/NONE)
    agent_id_t agent_id;         // Agent ID (-1 for system events)
    audit_intent_action_t intent_action; // Optional intent action (-1 or INTENT_MAX if not applicable)
    unsigned int sequence;       // Sequence counter for chronological ordering
    char message[AUDIT_MSG_MAX]; // Fixed-size message buffer (null-terminated)
} audit_event_t;

// Initialize the audit system
void audit_init(void);

// Emit an audit event (structured record)
// Parameters:
//   type: Event type enum
//   result: Result enum (AUDIT_RESULT_NONE, AUDIT_RESULT_ALLOW, AUDIT_RESULT_DENY, AUDIT_RESULT_SUCCESS, AUDIT_RESULT_FAILURE)
//   agent_id: Agent ID (-1 for system events)
//   intent_action: Intent action (-1 if not applicable, otherwise the intent action value)
//   message: Event message (null-terminated)
// Returns: 0 on success, -1 on failure (buffer full or invalid args)
int audit_emit(audit_type_t type, audit_result_t result, agent_id_t agent_id, audit_intent_action_t intent_action, const char* message);

// Dump all audit events to VGA console in chronological order (oldest→newest)
void audit_dump_to_console(void);

#endif // AUDIT_H
