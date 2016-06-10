set(SOURCES
    csm.c)


set(HEADERS 
    csm_defs.h
    csm.h)

add_library(csm STATIC ${SOURCES} ${HEADERS})

add_executable(csm_sample sample.c)

target_link_libraries(csm_sample csm)

install(TARGETS csm DESTINATION /usr/lib)

