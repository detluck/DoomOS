/*
 * kernel/include/defs.h
 *
 * PURPOSE:
 * Project-wide helper macros, constants, and compiler attributes.
 *
 */

#pragma once

#include <stddef.h>

/*
 * Compiler Attributes
 */

// Prevents compiler from adding padding bytes
#define __packed __attribute__((packed))

// Tells compiler this function never returns
#define __noreturn __attribute__((noreturn))

// Forces compiler to inline the function
#ifndef __always_inline
#define __always_inline inline __attribute__((always_inline))
#endif

// Silences -Wunused-parameter warnings in starter stubs
#define __unused __attribute__((unused))

// Forces a variable or struct to align to a byte boundary
#define __aligned(n) __attribute__((aligned(n)))

/*
 *  Memory manipulation
 */

#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

/*
 * Utility macros
 */

#define container_of(ptr, type, member) \
    ({ \
        const typeof(((type *) 0)->member) *__mptr = (ptr); \
        (type *) ((char *) __mptr - offsetof(type, member)); \
    })
