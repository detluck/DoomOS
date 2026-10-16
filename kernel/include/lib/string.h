/*
 * kernel/include/lib/string.h
 *
 * PURPOSE:
 * Basic memory manipulation and string operations for freestanding kernel
 * environment.
 *
 * WHAT SHOULD GO HERE:
 * 1. Raw memory operations:
 *    - void *memset(void *dest, int val, size_t count);
 *      Fills memory with a byte value. Essential for zeroing page tables and
 * buffers.
 *    - void *memcpy(void *dest, const void *src, size_t count);
 *      Copies 'count' bytes from src to dest.
 *    - int memcmp(const void *s1, const void *s2, size_t count);
 *      Compares two memory buffers.
 *
 * 2. String operations:
 *    - size_t strlen(const char *str);
 *      Calculates the length of a null-terminated string.
 *    - int strcmp(const char *s1, const char *s2);
 *      Compares two strings. Essential for shell command parsing.
 *    - int strncmp(const char *s1, const char *s2, size_t n);
 *      Compares up to n characters of two strings.
 */
