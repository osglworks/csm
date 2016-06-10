#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "csm_test.h"

static void output(char * buf, csm_state_id_t * array, size_t num) {
    int i;
    for (i = 0; i < num; ++i) {
        if (CSM_STATE_ID_UPPER_BOUND == array[i]) {
            break;
        }
        sprintf(buf, "%d", (int) array[i]);
    }
}

void csm_assert_snapshot(const csm_state_machine_t * machine, size_t num, ...) {
    csm_state_id_t expected[num + 1];
    csm_state_id_t snapshot[num + 1];
    int i;
    va_list states;
    va_start(states, num);
    for (i = 0; i < num; ++i) {
        expected[i] = va_arg(states, csm_state_id_t);
        snapshot[i] = CSM_STATE_ID_UPPER_BOUND;
    }
    expected[num] = CSM_STATE_ID_UPPER_BOUND;
    snapshot[num] = CSM_STATE_ID_UPPER_BOUND;
    csm_take_snapshot(machine, snapshot);
    if (CSM_STATE_ID_UPPER_BOUND != snapshot[num]) {
        ck_assert_msg(FALSE, "Actual states path is longer than expected");
        return;
    }
    if (0 != memcmp(expected, snapshot, num + 1)) {
        char expected_str[num];
        char found_str[num];
        output(expected_str, expected, num);
        output(found_str, snapshot, num);
        ck_assert_msg(FALSE, "State path expected: %s, state path found: %s", expected_str, found_str);
    }
}