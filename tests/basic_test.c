#include <stdlib.h>
#include <check.h>
#include "../src/csm.h"
#include "check_types.h"
#include "csm_test.h"


typedef enum {
    ST_ON, ST_OFF
} state_id_t;

typedef enum {
    TURN_ON, TURN_OFF
} event_id_t;


static csm_state_t states[] = {
        {
                .id = ST_OFF
        },
        {
                .id = ST_ON
        }
};

static csm_transition_t transitions[] = {
        {
                .event = TURN_ON,
                .from = states + ST_ON,
                .to = states + ST_OFF
        },
        {
                .event = TURN_OFF,
                .from = states + ST_OFF,
                .to = states + ST_ON
        }
};

static csm_state_machine_t machine = {
        .states = states,
        .state_count = 2,
        .transitions = transitions,
        .transition_count = 2
};

START_TEST(init_state_shall_be_first_state_in_list)
{

    csm_state_machine_return_t status = csm_init(&machine, NULL);
    ck_assert_msg(status == CSM_MACHINE_OK, "Was expecting csm_init return %d, found %s", CSM_MACHINE_OK, status);
    csm_assert_snapshot(&machine, 1, ST_OFF);
}
END_TEST

START_TEST(known_event_shall_trigger_state_transfer)
{
    csm_init(&machine, NULL);
    csm_simple_run(&machine, TURN_ON, NULL);
    csm_assert_snapshot(&machine, 1, ST_ON);
}
END_TEST

START_TEST(unknown_event_shall_trigger_state_transfer)
{
    csm_init(&machine, NULL);
    csm_state_machine_return_t status = csm_simple_run(&machine, TURN_OFF, NULL);
    ck_assert_int_eq(CSM_MACHINE_ERROR_UNKNOWN_EVENT, status);
    csm_assert_snapshot(&machine, 1, ST_OFF);
}
END_TEST


Suite * csm_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("csm");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, init_state_shall_be_first_state_in_list);
    tcase_add_test(tc_core, known_event_shall_trigger_state_transfer);
    suite_add_tcase(s, tc_core);

    return s;
}


int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = csm_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}