/*
 * kernel/include/defs.h
 *
 * PURPOSE:
 * Project-wide helper macros, constants, and compiler attributes.
 *
 */

#pragma once

/*
 * Compiler Attributes
 */

// Prevents compiler from adding padding bytes
#define __packed __attribute__((packed))

// Tells compiler this function never returns
#define __noreturn __attribute__((noreturn))

// Forces compiler to inline the function
#define __always_inline inline __attribute__((always_inline))

// Silences -Wunused-parameter warnings in starter stubs
#define __unused __attribute__((unused))

// Forces a variable or struct to align to a byte boundary
#define __aligned(n) __attribute__((aligned(n)))
