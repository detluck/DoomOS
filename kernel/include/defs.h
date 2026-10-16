/*
 * kernel/include/defs.h
 *
 * PURPOSE:
 * Project-wide helper macros, constants, and compiler attributes.
 *
 * WHAT SHOULD GO HERE:
 * 1. Compiler attributes:
 *    - __packed:        Tells GCC not to add padding between struct members (essential for GDT, IDT, hardware tables)
 *    - __noreturn:      Tells GCC a function never returns (used for panic(), halt())
 *    - __always_inline: Forces inlining for fast functions (like inb, outb)
 *
 * 2. Memory alignment and paging constants:
 *    - PAGE_SHIFT: 12 (since 2^12 = 4096)
 *    - PAGE_SIZE:  4096UL (size of standard x86-64 frame)
 *    - PAGE_MASK:  (~(PAGE_SIZE - 1))
 *    - ALIGN_UP(addr, align):   Rounds an address up to the nearest multiple of align
 *    - ALIGN_DOWN(addr, align): Rounds an address down to the nearest multiple of align
 *
 * 3. Math and bitwise utilities:
 *    - BIT(n):               Calculates (1ULL << (n))
 *    - ARRAY_SIZE(arr):      Calculates number of elements in a static array
 *    - DIV_ROUND_UP(n, d):   Integer division rounding up
 *    - barrier():            Instruction barrier to prevent compiler reordering
 */
