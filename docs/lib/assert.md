# Kernel Assert & Panic (`lib/assert`)

Runtime assertion checking and kernel panic handling for freestanding x86-64.

- **Header**: `kernel/include/lib/assert.h`
- **Source**: `kernel/lib/assert.c`

---

## Purpose

Enforces invariants and handles fatal errors. If an invariant fails, it prints a red alert to the VGA console, clears interrupts (`cli`), and permanently halts the CPU (`hlt`) to prevent cascading corruption or triple faults.

---

## API

- **`assert(condition)`**  
  Checks `condition`. If false (`0`), invokes `panic()` with the stringified expression.
- **`panic(const char *message)`**  
  Prints `KERNEL PANIC`, displays `message`, and enters an infinite `cli; hlt` halt loop. Marked `__noreturn`.

---

## Usage

```c
#include "lib/assert.h"

void init_paging(void *addr)
{
    // Panic if invariant fails
    assert(addr != NULL);
    assert(((uintptr_t)addr & 0xFFF) == 0); // Must be 4KB aligned

    // Explicit panic on fatal state
    if (!setup_tables(addr)) {
        panic("Failed to allocate paging tables");
    }
}
```
