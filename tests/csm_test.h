#ifndef CSM_TEST_H
#define CSM_TEST_H

#include <check.h>
#include "../src/csm.h"
#include "check_types.h"


#ifdef __cplusplus
extern "C" {
#endif


void csm_assert_snapshot(const csm_state_machine_t * machine, size_t num, ...);

#ifdef __cplusplus
}
#endif


#endif /* CSM_TEST_H */
