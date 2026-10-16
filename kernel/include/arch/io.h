/*
 * kernel/include/arch/io.h
 *
 * PURPOSE:
 * Hardware port I/O wrappers for x86-64 architecture.
 *
 * WHAT SHOULD GO HERE:
 * 1. Byte-level port I/O:
 *    - static inline u8 inb(u16 port);
 *      Reads 1 byte from the specified hardware I/O port using inline assembly
 * ("inb %1, %0").
 *    - static inline void outb(u16 port, u8 val);
 *      Writes 1 byte to the specified hardware I/O port using inline assembly
 * ("outb %0, %1").
 *
 * 2. Word and double-word port I/O (optional / when needed):
 *    - inw / outw (16-bit)
 *    - inl / outl (32-bit)
 *
 * 3. Hardware delay helper:
 *    - static inline void io_wait(void);
 *      Writes to an unused I/O port (typically 0x80) to give slow legacy
 * hardware time to settle.
 *
 * USAGE:
 * - Used by PS/2 keyboard driver (ports 0x60, 0x64).
 * - Used by PIC 8259 interrupt controller remap (ports 0x20, 0x21, 0xA0, 0xA1).
 * - Used by PIT 8254 system timer (ports 0x40, 0x43).
 * - Used by serial port debugging (port 0x3F8).
 */
