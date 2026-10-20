# Kernel Logging (`lib/log`)

Color-coded VGA logging with subsystem scopes for early boot diagnostics.

- **Header**: `kernel/include/lib/log.h`
- **Source**: `kernel/lib/log.c`

---

## Purpose

Standardizes early boot messages into colored status badges (`[ LEVEL ] [ SCOPE ] Message`) directly on the VGA console without requiring a full `printf` implementation.

---

## Log Levels

| Shorthand | Function | Badge | Color | Usage |
|---|---|---|---|---|
| `pr_ok(msg)` | `log_ok(scope, msg)` | `[ OK ]` | Light Green | Successful milestone / check |
| `pr_info(msg)` | `log_info(scope, msg)` | `[INFO]` | Light Cyan | System discovery / status info |
| `pr_warn(msg)` | `log_warn(scope, msg)` | `[WARN]` | Yellow | Non-fatal anomaly / fallback |
| `pr_err(msg)` | `log_error(scope, msg)` | `[ERR ]` | Light Red | Recoverable failure |

---

## Usage

Define `LOG_SCOPE` before `#include "lib/log.h"` (defaults to `"KERNEL"` if omitted):

```c
#define LOG_SCOPE "BOOT"
#include "lib/log.h"

void boot_init(void)
{
    pr_ok("Multiboot2 verified.");     // [ OK ] [BOOT] Multiboot2 verified.
    pr_info("Found 128 MB RAM.");      // [INFO] [BOOT] Found 128 MB RAM.
    pr_warn("ACPI table missing.");    // [WARN] [BOOT] ACPI table missing.
    pr_err("Out of memory!");          // [ERR ] [BOOT] Out of memory!
}
```

Or call directly with an explicit scope:
```c
log_info("PMM", "Page frame allocator initialized.");
```
