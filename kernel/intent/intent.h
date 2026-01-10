// AgentOS Intent Module
// Week 2 Day 1: Intent-based action system

#ifndef INTENT_H
#define INTENT_H

#include "cap/cap.h"  // For cap_mask_t

// Maximum intent payload size (including null terminator)
#define INTENT_PAYLOAD_MAX 128

// Intent action types
typedef enum {
    INTENT_CONSOLE_WRITE = 0,
    // Future intent actions can be added here:
    // INTENT_FILE_READ,
    // INTENT_NETWORK_CONNECT,
    INTENT_MAX  // Sentinel value
} intent_action_t;

// Intent structure
typedef struct {
    intent_action_t action;                  // Intent action type
    char payload[INTENT_PAYLOAD_MAX];        // Fixed-size payload (null-terminated)
} intent_t;

// Map intent action to required capability mask
// Returns: capability mask required for the intent action, or CAP_NONE if unknown
static inline cap_mask_t intent_action_to_capability(intent_action_t action) {
    switch (action) {
        case INTENT_CONSOLE_WRITE:
            return CAP_CONSOLE_WRITE;
        default:
            return CAP_NONE;
    }
}

#endif // INTENT_H
