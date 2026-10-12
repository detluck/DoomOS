# Contributing Guidelines

Guidelines for collaborating on our x86-64 kernel project. Please read before submitting code.

---

## 1. Git Workflow & PRs

- **Branches**: Create feature branches off `main`:
  - `feat/<feature-name>` (e.g., `feat/gdt-setup`, `feat/vga-driver`)
  - `fix/<bug-description>` (e.g., `fix/page-fault-handler`)
  - `refactor/<module>`
- **No Direct Pushes to `main`, if you changed the code of someone else**: All changed code of others enters `main` via Pull Requests.
- **Commit Messages**: Follow Conventional Commits format (`type(scope): short description`):
  - `feat(scope)`: New features (e.g., `feat(vga): add color text printing support`)
  - `fix(scope)`: Bug fixes (e.g., `fix(idt): correct double fault handler offset`)
  - `refactor(scope)`: Code changes that neither fix bugs nor add features (e.g., `refactor(mm): simplify frame allocator`)
  - `docs(scope)`: Documentation changes (e.g., `docs(readme): add build instructions`)
  - Common scopes: `boot`, `vga`, `gdt`, `idt`, `mm`, `sched`, `ci`, `arch`, etc.
- **Reviews**: Every PR requires at least one teammate review and approval before merge.

---

## 2. C Code Style

We follow K&R / Linux kernel style conventions:

- **Indentation**: 4 spaces (no mixed tabs/spaces).
- **Line Length**: Max 100 characters per line.
- **Braces**:
  - Functions: Opening brace on a new line.
  - Control Flow (`if`, `for`, `while`, `switch`): Opening brace on the same line.
- **Naming**:
  - Functions & Variables: `snake_case` (e.g., `pmm_alloc_frame`, `page_count`)
  - Types & Structs: `snake_case_t` (e.g., `idt_entry_t`, `page_table_t`)
  - Macros & Constants: `UPPER_SNAKE_CASE` (e.g., `PAGE_SIZE_4K`, `CR0_PG_FLAG`)
  - Avoid single-letter variable names except for simple loop counters (`i`, `j`).
- **Headers**:
  - Use include guards (`#ifndef MODULE_H`, `#define MODULE_H`) or `#pragma once`.
  - Expose only necessary public interfaces; mark file-local functions/variables as `static`.

---

## 3. Kernel & Bare-Metal Best Practices

- **Freestanding C**:
  - No hosted standard library headers (e.g., `<stdio.h>`, `<stdlib.h>`).
  - Use freestanding headers: `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`.
  - Always use fixed-width integer types (`uint8_t`, `uint32_t`, `uint64_t`, `uintptr_t`).
- **Stack Usage**:
  - Kernel stack space is strictly limited. Avoid large arrays or structures on the stack.
- **Hardware & Memory Access**:
  - Mark Memory-Mapped I/O (MMIO) and hardware register pointers as `volatile`.
  - Keep inline assembly clean: encapsulate raw assembly instructions inside small, inline helper functions.
- **Error Handling**:
  - Explicitly check return values and error codes.
  - Assert assumptions during early boot and panic cleanly on unrecoverable states with helpful debug messages.

---

## 4. Build Commands (`make`)

The project uses GNU Make to automate compiling, linking, ISO generation, and testing:

| Command | Description |
| :--- | :--- |
| **`make`** (or `make all`) | Compiles all assembly and C sources, links the 64-bit kernel (`build/kernel.bin`), and generates `build/kernel32.bin` for QEMU direct booting. |
| **`make run`** | Builds the kernel and immediately launches it inside a QEMU virtual machine window. |
| **`make iso`** | Packages GRUB and the 64-bit kernel into a bootable ISO image (`build/kernel.iso`) for flashing to USB drives (requires `xorriso`). |
| **`make clean`** | Deletes the `build/` directory and all compiled binaries and object files. |

---

## 5. Verification Checklist

Before opening a PR, ensure:

1. [ ] Builds cleanly with zero warnings (`make`).
2. [ ] Tested and runs in emulator (`make run`) without panics or triple faults.
3. [ ] No dead code, obsolete commented code, or leftover debugging prints.
4. [ ] Commits follow Conventional Commits format (`type(scope): ...`).
