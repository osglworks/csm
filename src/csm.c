#include "csm.h"

typedef csm_state_t * state_ptr_t;
typedef csm_event_t * event_ptr_t;
typedef csm_transition_t * transition_ptr_t;
typedef csm_state_machine_t * machine_ptr_t;
typedef csm_config_t * config_ptr_t;

typedef struct lookup_node {
    csm_transition_t * transition;
    struct lookup_node * next;
} lookup_node_t;

/*
 * Determined by optimize hint and the state circumstance, it 
 * could use array or list to store transitions for a certain
 * source state:
 * * Optimize for space will always use lookup node linked list
 * * Auto optimize might use array to index transition by event ID
 *   when there are over 4 events triggered on a singe state
 */
typedef struct array_list {
    csm_transition_t * array;
    lookup_node_t * list;
} array_list_t;

typedef union lookup {
    csm_transition_t * * * table;
    array_list_t * array_list;
} lookup_t;

typedef struct csm_data {
    int max_state_id;
    int max_event_id;

    csm_optimize_hint_t optimize_hint;
    lookup_t * lookup;
    lookup_node_t * complete_transitions;

    const csm_state_t * entry_state;
    const csm_state_t * active_state;
    const csm_state_t * history_state;
    const csm_state_machine_t * parent;
} csm_data_t;

static csm_config_t DEF_CONFIG = {
    .get_buffer = &calloc,
    .free_buffer = &free,
    .optimize_hint = CSM_OPTIMIZE_AUTO
};

static csm_state_machine_return_t init_machine(
    csm_state_machine_t * const machine, 
    const csm_state_machine_t * const parent,
    csm_get_buffer_func_t get_buffer,
    csm_free_buffer_func_t free_buffer,
    const void * context
);

/* find out max state ID and recursively init sub machine */
static csm_state_machine_return_t init_scan_states(
    const csm_state_machine_t * machine,
    int * max_state_id,
    csm_get_buffer_func_t get_buffer,
    csm_free_buffer_func_t free_buffer,
    const void * context
) {
    csm_state_machine_return_t status = CSM_MACHINE_OK;
    int i;
    for (i = 0; i < machine->state_count; ++i) {
        csm_state_t state = machine->states[i];
        if (state.id < CSM_STATE_ID_UPPER_BOUND) {
            if (NULL != state.sub_machine) {
                status = init_machine(
                    state.sub_machine, 
                    machine,
                    get_buffer, 
                    free_buffer, 
                    context);
                if (CSM_MACHINE_OK != status) {
                    break;
                }
            }
            * max_state_id = MAX(* max_state_id, state.id);
        } else {
            status = CSM_MACHINE_ERROR_INIT_STATE_ID_OVERFLOW;
            break;
        }
    }
    if (CSM_MACHINE_OK == status && (* max_state_id) < 0) {
        status = CSM_MACHINE_ERROR_INIT_NO_STATE_FOUND;
    }
    return status;
}

static csm_state_machine_return_t init_scan_transitions(
    const csm_state_machine_t * const machine,
    int max_state_id,
    int * max_event_id
) {
    csm_state_machine_return_t status = CSM_MACHINE_OK;
    int i;
    for (i = 0; i < machine->transition_count; ++i) {
        const csm_transition_t * const transition = &machine->transitions[i];
        if (NULL == transition) {
            status = CSM_MACHINE_ERROR_FATAL;
            break;
        }
        if (transition->from->id > max_state_id) {
            status = CSM_MACHINE_ERROR_INIT_STATE_ID_OVERFLOW;
            break;
        }
        if (transition->to->id > max_state_id && transition->to->id != CSM_STATE_ID_FINAL) {
            status = CSM_MACHINE_ERROR_INIT_STATE_ID_OVERFLOW;
            break;
        }
        if (transition->event < CSM_EVENT_ID_UPPER_BOUND) {
            (* max_event_id) = MAX(* max_event_id, transition->event);
        } else if (transition->event != CSM_EVENT_ID_COMPLETE) {
            status = CSM_MACHINE_ERROR_INIT_EVENT_ID_OVERFLOW;
            break;
        }
    }
    if (CSM_MACHINE_OK == status && (* max_event_id) < 0) {
        status = CSM_MACHINE_ERROR_INIT_NO_TRANSITION_FOUND;
    }
    return status;
}

static csm_transition_t *** init__build_table(
    csm_state_machine_t * const machine,
    const int max_state_id, 
    const int max_event_id,
    csm_data_t * const data,
    const csm_get_buffer_func_t get_buffer
) {
    csm_transition_t *** table = get_buffer(max_event_id + 1, sizeof(int *));
    if (NULL == table) {
        return NULL;
    }
    int i;
    for (i = 0; i <= max_event_id; ++i) {
        table[i] = get_buffer(max_state_id + 1, sizeof(csm_transition_t *));
        if (NULL == table[i]) {
            return NULL;
        }
    }
    for (i = 0; i < machine->transition_count; ++i) {
        const csm_transition_t * const transition = &(machine->transitions[i]);
        int event = transition->event;
        int state = transition->from->id;
        if (event != CSM_EVENT_ID_COMPLETE) {
            table[event][state] = (csm_transition_t *)transition;
        } else {
            lookup_node_t * node = get_buffer(1, sizeof(lookup_node_t *));
            if (NULL == node) {
                return NULL;
            }
            node->transition = (csm_transition_t *)transition;
            if (NULL != data->complete_transitions) {
                node->next = data->complete_transitions;
            }
            data->complete_transitions = node;
        }
    }
    return table;
}

static array_list_t * init__build_array_list(
    const csm_state_machine_t * const machine,
    const csm_optimize_hint_t hint,
    const int max_state_id,
    const int max_event_id,
    csm_data_t * const data,
    const csm_get_buffer_func_t get_buffer,
    const csm_free_buffer_func_t free_buffer
) {
    array_list_t * al = get_buffer(max_state_id + 1, sizeof(array_list_t *));
    if (NULL == al) {
        return NULL;
    }
    int i, j;
    for (i = 0; i <= max_state_id; ++i) {
        int event_count = 0;
        csm_transition_t ** array = NULL;
        lookup_node_t * list = NULL;
        for (j = 0; j < machine->transition_count; ++i) {
            const csm_transition_t * const transition = &(machine->transitions[i]);
            if (transition->from->id != i) {
                continue;
            }
            if (transition->event == CSM_EVENT_ID_COMPLETE) {
                lookup_node_t * node = get_buffer(1, sizeof(lookup_node_t *));
                if (NULL == node) {
                    return NULL;
                }
                node->transition = (csm_transition_t *) transition;
                if (NULL != data->complete_transitions) {
                    node->next = data->complete_transitions;
                }
                data->complete_transitions = node;
                continue;
            }
            if (NULL != array) {
                array[transition->event] = (csm_transition_t *) transition;
                continue;
            }
            if (CSM_OPTIMIZE_AUTO == hint && ++event_count > 4) {
                /* convert list to array */
                array = get_buffer(max_event_id + 1, sizeof(csm_transition_t *));
                if (NULL == array) {
                    return NULL;
                }
                lookup_node_t * node = list;
                while (NULL != node) {
                    array[node->transition->event] = node->transition;
                    lookup_node_t * tmp = node;
                    node = node->next;
                    free_buffer(tmp);
                }
                array[transition->event] = (csm_transition_t *) transition;
            } else {
                lookup_node_t * node = get_buffer(1, sizeof(lookup_node_t *));
                node->transition = (csm_transition_t *) transition;
                if (NULL == node) {
                    return NULL;
                }
                if (NULL != list) {
                    node->next = list;
                }
                list = node;
            }
        }
    }
    return al;
}

static csm_state_machine_return_t init_build_machine(
    csm_state_machine_t * const machine,
    const csm_state_machine_t * const parent,
    int max_state_id,
    int max_event_id,
    csm_get_buffer_func_t get_buffer,
    csm_free_buffer_func_t free_buffer,
    const void * context
) {
    csm_state_machine_return_t status = CSM_MACHINE_OK;
    csm_optimize_hint_t hint = CSM_OPTIMIZE_AUTO;
    csm_config_t * config = machine->config;
    if (NULL != config) {
        hint = config->optimize_hint;
    }
    lookup_t * lookup = get_buffer(1, sizeof(lookup_t));
    if (NULL == lookup) {
        return CSM_MACHINE_ERROR_FATAL;
    }
    csm_data_t * data = get_buffer(1, sizeof(csm_data_t));
    if (CSM_OPTIMIZE_TIME == hint) {
        lookup->table = init__build_table(machine, max_state_id, max_event_id, data, get_buffer);
        if (CSM_MACHINE_ERROR_FATAL <= status || NULL == lookup->table) {
            return CSM_MACHINE_ERROR_FATAL;
        }
    } else {
        lookup->array_list = init__build_array_list(
            machine, hint, max_state_id, max_event_id, data, get_buffer, free_buffer);
    }
    data->max_state_id = max_state_id;
    data->max_event_id = max_event_id;
    data->optimize_hint = hint;
    data->lookup = lookup;
    data->entry_state = &machine->states[1];
    data->parent = parent;
    machine->csm_data = data;
    return CSM_MACHINE_OK;
}

static csm_state_machine_return_t init_machine(
    csm_state_machine_t * const machine, 
    const csm_state_machine_t * const parent,
    csm_get_buffer_func_t get_buffer,
    csm_free_buffer_func_t free_buffer,
    const void * context
) {
    if (NULL == machine) {
        return CSM_MACHINE_ERROR_FATAL;
    }
    if (machine->state_count < 1) {
        return CSM_MACHINE_ERROR_INIT_NO_STATE_FOUND;
    }
    if (machine->transition_count < 1) {
        return CSM_MACHINE_ERROR_INIT_NO_TRANSITION_FOUND;
    }

    int max_state_id = -1;
    csm_state_machine_return_t status = init_scan_states(
        machine, 
        &max_state_id, 
        get_buffer, 
        free_buffer, 
        context);

    if (CSM_MACHINE_OK != status) {
        return status;
    }

    int max_event_id = -1;
    status = init_scan_transitions(machine, max_state_id, &max_event_id);
    if (CSM_MACHINE_OK != status) {
        return status;
    }

    return init_build_machine(
        machine, 
        parent,
        max_state_id, 
        max_event_id, 
        get_buffer, 
        free_buffer,
        context);
}


static void init_config(csm_state_machine_t * const machine) {
    config_ptr_t config = (config_ptr_t) machine->config;
    if (NULL == config) {
        machine->config = &DEF_CONFIG;
    } else {
        if (NULL == config->get_buffer) {
            config->get_buffer = DEF_CONFIG.get_buffer;
        }
        if (NULL == config->free_buffer) {
            config->free_buffer = DEF_CONFIG.free_buffer;
        }
    }
}

/* ------------------------------------------------------------------------ */

static csm_transition_t * lookup_transition(
    const csm_data_t * const data,
    const csm_event_id_t const event
) {
    csm_state_id_t state = data->active_state->id;
    if (event == CSM_EVENT_ID_COMPLETE) {
        lookup_node_t * node = data->complete_transitions;
        while (NULL != node) {
            if (node->transition->from->id == state) {
                return node->transition;
            }
        }
    }
    if (event > data->max_event_id) {
        return NULL;
    }
    if (CSM_OPTIMIZE_TIME == data->optimize_hint) {
        return data->lookup->table[event][state];
    } else {
        array_list_t al = data->lookup->array_list[state];
        if (NULL != al.array) {
            return &al.array[event];
        } else {
            lookup_node_t * node = al.list;
            while (NULL != node) {
                if (event == node->transition->event) {
                    return node->transition;
                }
            }
            return NULL;
        }
    }
}

static csm_state_machine_return_t run_process_transition(
) {

}

static csm_state_machine_return_t run_trigger_complete_event(
    const csm_state_machine_t * const machine,
    const csm_event_t * const event,
    void * const context
) {
    csm_data_t* data = machine->csm_data;
    csm_transition_t * transition = lookup_transition(data, event->id);
    if (NULL != transition) {
        // TODO process transition
    }
}

static csm_state_machine_return_t run_enter_state(
    const csm_state_machine_t * const machine,
    const csm_state_t * const target,
    const csm_event_t * const event,
    const boolean restore_history,
    const csm_history_type_t history,
    void * const context
);

static csm_state_machine_return_t run_restore_history(
    const csm_state_machine_t * const machine,
    const csm_event_t * const event,
    const boolean restore_history,
    const csm_history_type_t history,
    void * const context
) {
    csm_data_t* data = machine->csm_data;
    if (NULL != data->history_state) {
        run_enter_state(
            machine, 
            data->history_state, 
            event, 
            restore_history, 
            history, 
            context);
    }
}

static csm_state_machine_return_t run_enter_state(
    const csm_state_machine_t * const machine,
    const csm_state_t * const target,
    const csm_event_t * const event,
    const boolean restore_history,
    const csm_history_type_t history,
    void * const context
) {

    csm_data_t* data = machine->csm_data;

    if (CSM_STATE_ID_FINAL == target->id) {
        /* 
         * we have reached final state of this machine
         * let's trigger the COMPLETE event
         * on enclosing parent state
         */
        if (NULL == data->parent) {
            /* this is the top level state machine */
            return CSM_MACHINE_OK;
        }

        return run_trigger_complete_event(data->parent, event, context);
    }

    if (NULL != target->on_enter) {
        csm_action_return_t status = target->on_enter(event, context);
        if (CSM_ACTION_OK != status) {
            return CSM_MACHINE_ERROR_FATAL;
        }
    }

    data->active_state = target;

    if (!restore_history || NULL == target->sub_machine) {
        return CSM_MACHINE_OK;
    }

    boolean deep_history = CSM_HISTORY_DEEP == history;
    run_restore_history(target->sub_machine, event, deep_history, history, context);
}

static csm_state_machine_return_t run_handle_event(
    const csm_state_machine_t * const machine,
    const csm_event_t * const event,
    void * const context
);


/* ------------------------------------------------------------------------ */

/*
 * CSM defined public data 
 */
csm_event_t CSM_EVENT_TERMINATE = {
    .id = CSM_EVENT_ID_TERMINATE    
};

csm_event_t CSM_EVENT_COMPLETE = {
    .id = CSM_EVENT_ID_COMPLETE
};

csm_event_t CSM_EVENT_INIT = {
    .id = CSM_EVENT_ID_INIT
};

csm_state_t CSM_STATE_FINAL = {
    .id = CSM_STATE_ID_FINAL,
    .name = "final"
};

/*
 * public functions
 */

csm_state_machine_return_t csm_state_machine_init(
    csm_state_machine_t * const machine, 
    const void * context) 
{
    init_config(machine);
    return init_machine(
        machine, 
        NULL,
        machine->config->get_buffer, 
        machine->config->free_buffer, 
        context);
}