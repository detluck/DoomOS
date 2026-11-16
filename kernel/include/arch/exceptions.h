#pragma once

#include "arch/interrupt_context.h"
#include "defs.h"

#define EXCEPTION_COUNT 32
#define EXCEPTION_DOUBLE_FAULT 8
#define EXCEPTION_GENERAL_PROTECTION 13
#define EXCEPTION_PAGE_FAULT 14

/* fault_address is an early CR2 snapshot, meaningful only for #PF.
 * entry_rsp is the actual dispatcher entry stack pointer captured by assembly.
 */
void exception_panic(const interrupt_context_t *context, u64 fault_address,
                     uintptr_t entry_rsp) __noreturn;
