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

#define assert(condition)                                                                          \
  do {                                                                                             \
    if (!(condition)) {                                                                            \
      panic("Assertion failed: " #condition);                                                      \
    }                                                                                              \
  } while (0)
