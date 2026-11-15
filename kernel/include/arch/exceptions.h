#pragma once

#include "arch/interrupt_context.h"
#include "defs.h"

#define EXCEPTION_COUNT 32
#define EXCEPTION_PAGE_FAULT 14

/* fault_address is an early CR2 snapshot, meaningful only for #PF. */
void exception_panic(const interrupt_context_t *context, u64 fault_address) __noreturn;
