find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})

set(TEST_SOURCES
  basic_test.c
  csm_test.c
)

set(TEST_HEADERS
    check_types.h
    csm_test.h
)

set(EXTRA_LIBS m rt pthread)

add_executable(check_csm ${TEST_SOURCES} ${TEST_HEADERS})
target_link_libraries(check_csm csm ${CHECK_LIBRARIES} ${EXTRA_LIBS})
