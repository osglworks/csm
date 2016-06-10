#ifndef CSM_DEFS_H
#define CSM_DEFS_H

/*
 * This file defines generic types/values/macros
 * used in CSM library
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef boolean
typedef char boolean;
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef TRUE
#define TRUE ((boolean)1)
#endif

#ifndef FALSE
#define FALSE ((boolean)0)
#endif

#ifndef MAX
#define MAX(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })
#endif

#ifdef __cplusplus
}
#endif


#endif /* CSM_DEFS_H */
