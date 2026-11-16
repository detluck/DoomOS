#pragma once

#include "defs.h"

/* Present only in an explicitly selected ARCH_TEST build. */
void arch_selftest_run(void) __noreturn;
