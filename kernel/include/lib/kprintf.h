/*
 * kernel/include/lib/kprintf.h
 *
 * PURPOSE:
 * Minimal formatted output to the VGA console.
 *
 * SUPPORTED FORMAT:
 * %c %s %d %u %x %p %%
 * Prefix d/u/x with 'l' or 'll' for 64-bit values (e.g. %lu, %llx).
 */

#pragma once

void kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
