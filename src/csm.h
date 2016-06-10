#ifndef CSM_H
#define CSM_H

/*
 * This fine declared public data structures and functions
 * provided by CSM library to user application
 */

#include <stdlib.h>
#include "csm_defs.h"

#ifdef __cplusplus
extern "C" {
#endif


/* identify event across all statemachine hirerachies */
typedef size_t csm_event_id_t;

/*
 * reserved id for event: terminate
 * --------------------------------------------
 * NOTE: 
 * when statemachine encountered terminate event
 * it transit to terminate state and free all 
 * resources immediately
 */
#define CSM_EVENT_ID_TERMINATE ((csm_event_id_t)0XFFFF)

/*
 * reserved id for pseudo event: complete
 * ----------------------------------------------
 * when a sub state machine reached a final state 
 * CSM will generate a complete event on enclosing 
 * state in parent state machine
 *
 * Note unlike the terminate event, complete event
 * is NOT supposed to be feed into the state machine
 * by application. If CSM received event with this ID,
 * it will be dropped off immediately and the 
 * CSM_MACHINE_ERROR_UNKNWON_EVENT will be returned
 */
#define CSM_EVENT_ID_COMPLETE ((csm_event_id_t)0XFFFE)

/*
 * reserved id for pseudo event: init
 * ---------------------------------------
 * this event will be generated and feed into the 
 * state machine when state machine has been
 * initialized and trigger the entry state as
 * active state
 */
#define CSM_EVENT_ID_INIT ((csm_event_id_t)0XFFFD)

/*
 * App defined event ID shall NOT exceed this bound
 * Otherwise it will cause failure to initialize the
 * state machine
 */
#define CSM_EVENT_ID_UPPER_BOUND ((csm_event_id_t)0XF000)

/* 
 * the state structure 
 * -------------------------
 */
typedef struct csm_event {
    /*
     * event id
     * ---------------------
     * The ID of the event across entire state machine hierarchies
     * must be continous int starts from zero. 
     * 
     * It is recommended to use enum to define event IDs
     * in top level statemachine header file
     */
    const csm_event_id_t id;

    /*
     * event name
     * ----------------------
     * optional, could be useful when debugging the
     * state machine
     */
    /*@null@*/ const char * const name;

    /*
     * event payload
     * ----------------------
     * optional, will be passed to enter/exit actions 
     * and transition functions if provided
     */
    /*@null@*/ void * payload;
} csm_event_t;

/* CSM defined event: terminate */
extern csm_event_t CSM_EVENT_TERMINATE;

/* CSM defined event: init */
extern csm_event_t CSM_EVENT_INIT;

/* 
 * action (function) return code
 */
typedef enum {
    /*
     * Action completed without problem
     */
    CSM_ACTION_OK, 
    /*
     * Error occurred calling the action.
     * For exit and transition action, this will cause CSM
     * trigger CSM_MACHINE_ERROR_ACTION_ERROR return code;
     * For entry action, this will cause CSM terminate immediately
     */
    CSM_ACTION_ERROR,
    /*
     * Fatal error encountered and the state machine must
     * be terminated immediately
     */
    CSM_ACTION_FATAL
} csm_action_return_t;

/*
 * Guard function pointer type
 * -----------------------------
 * The guard function provide a tool to allow application to
 * do further check and prevent transition from happening 
 * if certain criteria is not met.
 * 
 * @param event the received event
 * @param context the statemachine execution context
 * @return true if transition is allowed, false otherwise
 */
typedef boolean (*csm_guard_func_t)(
    /*@null@*/ /*@unused@*/ const csm_event_t * const event,
    /*@null@*/ /*@unused@*/ void * context);

/*
 * entry/exit action function pointer type
 * ---------------------------------------
 * App could link at most one entry/exit function to a 
 * state so when CSM enter or exit a state, these function
 * could be called.
 *
 * @param event the received event
 * @param context the statemachine execution context
 * @return code of csm_action_return_t type
 */
typedef csm_action_return_t (*csm_action_func_t)(
    /*@null@*/ /*@unused@*/ const csm_event_t * const event,
    /*@null@*/ /*@unused@*/ void * const context);

/* identify state in a single statemachine hierarchy */
typedef size_t csm_state_id_t;

/* reserved id for pseudo state: final */
#define CSM_STATE_ID_FINAL ((csm_state_id_t)0XFFFE)

/*
 * App defined state ID shall NOT exceed this bound
 * otherwise it will cause state machine initialization
 * failure
 */
#define CSM_STATE_ID_UPPER_BOUND ((csm_state_id_t)0XF000)

/* 
 * the state structure 
 * -------------------------
 */
typedef struct csm_state {
    /*
     * state id
     * ---------------------
     * The ID of the state in a single state machine hierarchi
     * must be continous int starts from zero. 
     * 
     * It is recommended to use enum to define state IDs
     * in one machine hierarchical level
     */
    const csm_state_id_t id;

    /*
     * state name
     * ----------------------
     * optional, could be useful when debugging the
     * state machine
     */
    /*@null@*/ const char * const name;

    /* 
     * sub machine
     * -----------------------
     * optional
     */
    /*@null@*/ struct csm_state_machine * const sub_machine;

    /* 
     * entry function
     * ------------------------
     * optional
     */
    /*@null@*/ csm_action_func_t on_enter;

    /* 
     * exit function
     * ------------------------
     * optional
     */
    /*@null@*/ csm_action_func_t on_exit;
} csm_state_t;

/*
 * CSM defined state: FINAL
 */ 
extern csm_state_t CSM_STATE_FINAL;

/*
 * Defines three types of transition
 * configuration in regarding to history.
 * 
 * Note, history type setting only effect
 * when transition target is a sub statemachine
 */
typedef enum {
    /*
     * do NOT restore history. Go straght to entry state
     */
    CSM_HISTORY_NONE,

    /*
     * restore history of the current state machine hirerachical
     * level. do not restore the history state contained in sub 
     * statemachine(s)
     */
    CSM_HISTORY_SHALLOW,

    /*
     * restore history of all statemachine hierarchical levels
     */
    CSM_HISTORY_DEEP
} csm_history_type_t;


/*
 * transition function pointer
 * ------------------------------------
 * @param event the received event
 * @param context the context
 * @param target allow the transaction function to manipulate
 *        transition target state
 * @return code of csm_action_return_t
 */
typedef csm_action_return_t (*csm_transition_func_t)(
    /*@null@*/ /*@unused@*/ const csm_event_t * const event,
    /*@null@*/ /*@unused@*/ void * context,
    const csm_state_t * target);


/*
 * transition data structure
 * -----------------------------
 * A transition is defined by
 * - event that triggers the transtion
 * - a source state (from where)
 * - a target state (to where)
 * - optional guard function
 * - optional transition action function
 * - the history setting. If not set then default is do not restore history
 */
typedef struct {
    /*
     * specify ID of the event that trigger this transition
     */
    const csm_event_id_t event;

    /*
     * the source state
     */
    const csm_state_t * const from;

    /*
     * the target state 
     */
    const csm_state_t * const to;

    /*
     * the guard function
     * ---------------------
     * optional, when specified then CSM will
     * call the fuction to check if transition is
     * allowed
     */
    /*@null@*/ const csm_guard_func_t guard;


    /*
     * the transition action function
     * ---------------------------------
     * optional, when specified then CSM will
     * call the function before exit current state
     *
     * Note this function will get called before
     * CSM exit the current state, so that if the
     * calling to this function has error return
     * CSM will stop transition and keep the state
     */
    /*@null@*/ const csm_transition_func_t action;

    /*
     * history setting of the transition
     */ 
    csm_history_type_t history;
} csm_transition_t;

/*
 * get buffer function pointer
 * --------------------------------------------------
 * After calling this function, the allocated buffer
 * shall be initialized to zero
 * @param item_count number of items in the buffer
 * @param item_size size of a single item
 * @return a pointer to the buffer been allocated
 */
typedef void * (* csm_get_buffer_func_t) (size_t item_count, size_t item_size);

/*
 * free buffer function pointer
 * @param buf the pointer to the buffer
 */
typedef void (* csm_free_buffer_func_t) (void * buf);

/* 
 * Statemachine destructor function pointer type
 * ------------------------------------------------
 * The statemachine destructor allows application to inject
 * logic when statemachine is terminated
 *
 * @param context the statemachine execution context
 */
typedef void (* csm_destructor_func_t)(/*@null@*/ /*@unused@*/ void * context);

/*
 * Option hints help CSM to choose transition lookup 
 * table data structure
 */
typedef enum {
    /*
     * Let CSM decide whether optimize
     * transition lookup by space or
     * by time
     * 
     * This is the recommended setting for
     * most state machine types
     */
    CSM_OPTIMIZE_AUTO,
    /*
     * Optimize transition lookup by
     * time. 
     *
     * Warning: this will cause CSM
     * generate (state_count * event_count) size
     * array for each state machine hierarchy
     * and because event is global across all
     * hierarchies so it is very space ineffective
     * do NOT use this in a app with memory 
     * constraints.
     */
    CSM_OPTIMIZE_TIME,
    /*
     * Optimize transition lookup by
     * space
     *
     * This should be a good choice for most
     * statemachine types. However if you have
     * a certain state that has many outbound
     * transitions (which is not a usual case)
     * a better choice might be CSM_OPTIMIZE_AUTO
     */
    CSM_OPTIMIZE_SPACE
} csm_optimize_hint_t;

/*
 * Global configuration
 * used to initialize a
 * state machine
 */
typedef struct csm_config {
    /*
     * get buffer function
     * -----------------------------
     * If not specified, then it will use the config
     * from the parent state machine. 
     * If not specified in the top level statemachine,
     * then standard calloc(size_t) will be called
     */
    csm_get_buffer_func_t get_buffer;
    /*
     * free buffer function
     * ----------------------------------
     * If not specified, then it will use the config
     * from the parent state machine
     * If not specified in the top level statemachine
     * then standard free(void *) will be called
     */
    csm_free_buffer_func_t free_buffer;

    /*
     * statemachine destructor
     * ----------------------------
     * optional hook function to be called when the statemachine is 
     * terminated if it is specified.
     * *Note* it will NOT call the parent config if sub statemachine
     * destructor is not specified
     */
    csm_destructor_func_t const destructor;

    /*
     * optimize hint
     * --------------------------------------------
     * Optional setting to tell CSM how to optimize
     * transition lookup.
     * If not specified, then CSM_OPTIMIZE_AUTO will
     * be assumed.
     * *Note* it will NOT look for parent config for
     * this setting if not specified in child statemachine     
     */
    csm_optimize_hint_t optimize_hint;

} csm_config_t;

/* the state machine data structure */
typedef struct csm_state_machine {
    /*
     * array of states in the toplevel hierarchi 
     * of this machine.
     * 
     * The first state in the array is always the
     * ENTRY state of the state machine. 
     *
     * If the state machine is a sub state machine 
     * CSM will always pass through the event trigger
     * the parent machine transition to the 
     * linked state to the entry state of this machine 
     */
    const csm_state_t * const states;
    
    /*
     * number of state in the state array
     */
    const size_t state_count;

    /*
     * array of transitions applied to all
     * states in the top level hierarchi
     * of this machine
     */
    const csm_transition_t * const transitions;

    /*
     * number of transitions in the transition array
     */
    const size_t transition_count;

    /*
     * Optional config settings to help CSM
     * initialize and run the statemachine
     */
    csm_config_t * config;

    /* 
     * placeholder for CSM internal data 
     * ---------------------------------
     * Warning, app shall NOT put anything here
     * this pointer is reserved for CSM use only
     * and will be written by CSM when initializing
     * the CSM
     */
    struct csm_data * csm_data;
} csm_state_machine_t;

/*
 * Return code from csm public functions
 */
typedef enum {
    /*
     * Everything is fine
     */
    CSM_MACHINE_OK,

    /*
     * The incoming event is unknown to the
     * current state or the entire state machine
     */
    CSM_MACHINE_ERROR_UNKNOWN_EVENT,

    /*
     * App defined exit or transition action call returns
     * CSM_ACTION_ERROR code
     */
    CSM_MACHINE_ERROR_ACTION_ERROR,

    /* if fatal error encountered, the machine shutdown immediately */
    CSM_MACHINE_ERROR_FATAL,

    CSM_MACHINE_ERROR_INIT_NO_STATE_FOUND,

    CSM_MACHINE_ERROR_INIT_NO_TRANSITION_FOUND,

    CSM_MACHINE_ERROR_INIT_STATE_ID_OVERFLOW,

    CSM_MACHINE_ERROR_INIT_EVENT_ID_OVERFLOW,

    /* machine internal logic error, must be a bug */
    CSM_MACHINE_ERROR_MACHINE_ERROR
} csm_state_machine_return_t;

/* 
 * Initialize a state machine
 * @param machine pointer to app defined state machine
 * @param context pointer to app supplied execution context,
 *        which will be passed to app defined entry actions
 * @return the csm_state_machine_return_t type return code
 */
csm_state_machine_return_t csm_init(
    csm_state_machine_t * machine, 
    void * const context);

/* 
 * Send event to a state machine
 * @param machine pointer to state machine
 * @param event the incoming event to be fed into the machine
 * @param context pointer to app supplied execution context,
 *        will be passed to app defined entry/exit/transition actions
 * @return the csm_state_machine_return_t type return code
 */
csm_state_machine_return_t csm_run (
    const csm_state_machine_t * machine,
    csm_event_t const * event, 
    void * const context);

/*
 * Send event id <to></to> a state machine
 * @param machine pointer to the state machine
 * @param event the event id
 * @param context pointer to app supplied execution context.
          will be passed to app defined entry/exit/ranstion actions
 * @return the csm_state_machine_return_t type return code
 */
csm_state_machine_return_t csm_simple_run(
    const csm_state_machine_t * machine,
    csm_event_id_t event,
    void * const context);

/*
 * Take a snapshot of the statemachine. 
 * @param snapshot an array used to save active state list
 */
void csm_take_snapshot(const csm_state_machine_t * machine, csm_state_id_t snapshot[]);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef CSM_H */
