#ifndef AUTOMATA_H
#define AUTOMATA_H

#include <stdint.h>
#include <string.h>

// Internal maximum sizes
#define _AUTOMATA_MAX_STATES 32
#define _AUTOMATA_MAX_EVENTS 32
#define _AUTOMATA_MAX_TRANSITIONS 64

// Internal types and structures
typedef uint8_t _automata_state_t;
typedef uint8_t _automata_event_t;
typedef void (*_automata_action_t)(void);

typedef struct {
    const char* curr_state;
    const char* event;
    const char* next_state;
    _automata_action_t action;
} _AUTOMATA_Transition;

typedef struct {
    _automata_state_t current_state;
    _automata_state_t transitions[_AUTOMATA_MAX_STATES][_AUTOMATA_MAX_EVENTS];
    _automata_action_t actions[_AUTOMATA_MAX_STATES][_AUTOMATA_MAX_EVENTS];
    const char* state_names[_AUTOMATA_MAX_STATES];
    const char* event_names[_AUTOMATA_MAX_EVENTS];
    uint8_t state_count;
    uint8_t event_count;
} _AUTOMATA;

// Internal AUTOMATA instance and transition table
static _AUTOMATA _automata = {0};
static _AUTOMATA_Transition _transitions[_AUTOMATA_MAX_TRANSITIONS];
static int _transition_count = 0;

// Internal helper functions
static _automata_state_t _get_state_id(const char* name) {
    for (_automata_state_t i = 0; i < _automata.state_count; i++) {
        if (strcmp(_automata.state_names[i], name) == 0) return i;
    }
    _automata.state_names[_automata.state_count] = name;
    return _automata.state_count++;
}

static _automata_event_t _get_event_id(const char* name) {
    for (_automata_event_t i = 0; i < _automata.event_count; i++) {
        if (strcmp(_automata.event_names[i], name) == 0) return i;
    }
    _automata.event_names[_automata.event_count] = name;
    return _automata.event_count++;
}

// Set initial state
#define INITIAL(state) \
    void INIT(void) { \
        _automata.current_state = _get_state_id(#state); \
        for (int i = 0; i < _transition_count; i++) { \
            _automata_state_t curr = _get_state_id(_transitions[i].curr_state); \
            _automata_event_t evt = _get_event_id(_transitions[i].event); \
            _automata_state_t next = _get_state_id(_transitions[i].next_state); \
            _automata.transitions[curr][evt] = next; \
            _automata.actions[curr][evt] = _transitions[i].action; \
        } \
    }

// Add a transition to the AUTOMATA
#define TRANSITION(curr_state, event, next_state, action) \
    static void _action_##curr_state##_##event(void) { action; } \
    static void __attribute__((constructor)) _init_##curr_state##_##event(void) { \
        _transitions[_transition_count] = (_AUTOMATA_Transition){ \
            #curr_state, \
            #event, \
            #next_state, \
            _action_##curr_state##_##event \
        }; \
        _transition_count++; \
    }

// Process events
#define EVENT(event_name) do { \
    _automata_event_t event = _get_event_id(#event_name); \
    _automata_action_t action = _automata.actions[_automata.current_state][event]; \
    if (action) { \
        action(); \
        _automata.current_state = _automata.transitions[_automata.current_state][event]; \
    } \
} while(0)

#endif // AUTOMATA_H