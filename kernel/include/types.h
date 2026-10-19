/*
 * kernel/include/types.h
 *
 * PURPOSE:
 * Core data type definitions used across the entire operating system.
 *
 */

#pragma once

#include <stdbool.h> // IWYU pragma: export
#include <stddef.h>  // IWYU pragma: export
#include <stdint.h>  // IWYU pragma: export

// aliases like on linux
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// Memory Types
typedef u64 phys_addr_t;
typedef u64 virt_addr_t;
typedef u64 pfn_t;
