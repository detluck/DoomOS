/*
 * kernel/include/lib/kprintf.h
 *
 * PURPOSE:
 * Minimal formatted output. vsnprintf/snprintf format into a buffer (no hardware
 * dependency, host-testable); kprintf formats and writes the result to the VGA console.
 *
 * SUPPORTED FORMAT:
 * %c %s %d %u %x %b %p %%
 * Prefix d/u/x with 'l' or 'll' for 64-bit values (e.g. %lu, %llx).
 * %s with NULL prints "(null)"; %p with NULL prints "(nil)".
 *
 * LIMITS:
 * kprintf output is truncated to 255 characters.
 * vsnprintf follows C99: always NUL-terminates when size > 0 and returns the length
 * the full output would have had.
 */

#pragma once

#include <stdarg.h>

#include "types.h"

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
    __attribute__((format(printf, 3, 0)));
int snprintf(char *buf, size_t size, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
void kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
