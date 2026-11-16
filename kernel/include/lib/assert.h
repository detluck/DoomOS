/*
 * kernel/include/lib/assert.h
 *
 * PURPOSE:
 * Kernel debugging, panic handling, and runtime assertions.
 *
 */

#pragma once

#include "defs.h"

void panic(const char *message) __noreturn;

/* Single-CPU fatal diagnostics: begin disables IRQs and guards all output.
 * A repeated entry halts without touching the console again.
 */
void panic_begin(void);
void panic_halt(void) __noreturn;

#define assert(condition) \
    do { \
        if (!(condition)) { \
            panic("Assertion failed: " #condition); \
        } \
    } while (0)
