// AgentOS Audit Log Module
// Week 2 Day 1: Fixed-size ring buffer audit log

#ifndef AUDIT_H
#define AUDIT_H

// Maximum number of audit events in ring buffer
#define AUDIT_MAX_EVENTS 64

// Maximum audit message length (including null terminator)
#define AUDIT_MSG_MAX 128

// Agent ID type (matches agent module)
typedef int agent_id_t;

// Audit event type
typedef enum {
    AUDIT_TYPE_AGENT_CREATED = 0,
    AUDIT_TYPE_AGENT_STARTED,
    AUDIT_TYPE_AGENT_COMPLETED,
    AUDIT_TYPE_AGENT_ERROR,
    AUDIT_TYPE_SYSTEM_INIT,
    AUDIT_TYPE_SYSTEM_ERROR,
    AUDIT_TYPE_USER_ACTION,
    AUDIT_TYPE_MAX               // Sentinel value
} audit_type_t;

// Audit event structure
typedef struct {
    audit_type_t type;           // Event type
    agent_id_t agent_id;         // Agent ID (-1 for system events)
    char message[AUDIT_MSG_MAX]; // Event message (null-terminated)
    unsigned int sequence;       // Sequence number for ordering
} audit_event_t;

// Initialize the audit system
void audit_init(void);

// Emit an audit event
// Returns: 0 on success, -1 on failure (buffer full or invalid args)
int audit_emit(audit_type_t type, agent_id_t agent_id, const char* message);

// Dump all audit events to VGA console in chronological order (oldest→newest)
void audit_dump_to_console(void);

#endif // AUDIT_H
