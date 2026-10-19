/*
 * kernel/include/arch/io.h
 *
 * PURPOSE:
 * Hardware port I/O wrappers for x86-64 architecture.
 */

#pragma once

#include "defs.h"
#include "types.h"

// 8-Bit Port
static __always_inline u8 inb(u16 port)
{
    u8 value;
    __asm__ volatile("inb %w1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static __always_inline void outb(u16 port, u8 value)
{
    __asm__ volatile("outb %b0, %w1" : : "a"(value), "Nd"(port));
}

// 16-Bit Port
static __always_inline u16 inw(u16 port)
{
    u16 value;
    __asm__ volatile("inw %w1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static __always_inline void outw(u16 port, u16 value)
{
    __asm__ volatile("outw %w0, %w1" : : "a"(value), "Nd"(port));
}

// 32-Bit Port
static __always_inline u32 inl(u16 port)
{
    u32 value;
    __asm__ volatile("inl %w1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static __always_inline void outl(u16 port, u32 value)
{
    __asm__ volatile("outl %0, %w1" : : "a"(value), "Nd"(port));
}

// hardware Delay
static __always_inline void io_wait(void)
{
    outb(0x80, 0);
}
