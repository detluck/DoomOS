/*
 * kernel/include/types.h
 *
 * PURPOSE:
 * Core data type definitions used across the entire operating system.
 *
 * WHAT SHOULD GO HERE:
 * 1. Freestanding compiler headers:
 *    - <stdint.h> (for fixed-width integer types)
 *    - <stddef.h> (for size_t, ptrdiff_t, NULL)
 *    - <stdbool.h> (for bool, true, false)
 *
 * 2. Shorthand integer aliases (Linux kernel style):
 *    - Unsigned: u8, u16, u32, u64
 *    - Signed:   s8, s16, s32, s64 (or i8, i16, i32, i64)
 *
 * 3. Kernel domain-specific memory types:
 *    - phys_addr_t: 64-bit unsigned integer representing raw physical RAM addresses
 *    - virt_addr_t: 64-bit unsigned integer representing virtual memory addresses
 *    - pfn_t:       64-bit unsigned integer representing a Page Frame Number (phys_addr / PAGE_SIZE)
 */
