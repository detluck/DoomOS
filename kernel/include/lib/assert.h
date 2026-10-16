/*
 * kernel/include/lib/assert.h
 *
 * PURPOSE:
 * Kernel debugging, panic handling, and runtime assertions.
 *
 * WHAT SHOULD GO HERE:
 * 1. Panic function prototype:
 *    - void panic(const char *fmt, ...) __attribute__((noreturn));
 *      Prints a red diagnostic message, optionally registers/backtrace, and
 * halts the CPU with cli + hlt.
 *
 * 2. Assert macro:
 *    - #define assert(condition)
 *      If condition is false, triggers a panic displaying:
 *      - The failed expression (using stringification: #condition)
 *      - Source filename (__FILE__)
 *      - Line number (__LINE__)
 *
 * USAGE:
 * - Validate assumptions during early boot (e.g. assert(frame != NULL)).
 * - Catches bugs immediately at the source instead of silent crashes.
 */
