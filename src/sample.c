#include "csm.h"
#include <stdio.h>

typedef enum {
    ST_ON, ST_OFF
} state_id_t;

typedef enum {
    TURN_ON, TURN_OFF
} event_id_t;

static boolean light = FALSE;

static csm_action_return_t turnOnLight(
        const csm_event_t * const event,
        void * context
) {
    boolean * light_ptr = (boolean *) context;
    * light_ptr = TRUE;
    return CSM_ACTION_OK;
}

static csm_action_return_t turnOffLight(
        const csm_event_t * const event,
        void * context
) {
    boolean * light_ptr = (boolean *) context;
    * light_ptr = FALSE;
    return CSM_ACTION_OK;
}

static csm_state_t states[] = {
        {
                .id = ST_OFF,
                .on_enter = &turnOffLight
        },
        {
                .id = ST_ON,
                .on_enter = &turnOnLight
        }
};

static csm_transition_t transitions[] = {
        {
                .event = TURN_ON,
                .from = states,
                .to = states + 1
        },
        {
                .event = TURN_OFF,
                .from = states + 1,
                .to = states
        }
};

static csm_state_machine_t machine = {
        .states = states,
        .state_count = 2,
        .transitions = transitions,
        .transition_count = 2
};

int main(void) {
    csm_state_machine_return_t status = csm_init(&machine, &light);
    printf("%d\n", light);
    csm_simple_run(&machine, TURN_ON, &light);
    printf("%d\n", light);
    csm_simple_run(&machine, TURN_ON, &light);
    printf("%d\n", light);
    csm_simple_run(&machine, TURN_OFF, &light);
    printf("%d\n", light);
}