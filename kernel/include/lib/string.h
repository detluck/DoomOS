/*
 * kernel/include/lib/string.h
 *
 * PURPOSE:
 * Memory and string manipulation functions for freestanding kernel environment.
 */

#pragma once

#include "types.h"

/*
 * Memory Manipulation (Raw byte operations)
 */

/**
 * memset - Fill a region of memory with a given byte value
 * @dest: Pointer to the start of the memory area
 * @val: The byte value to fill the area with
 * @n: Number of bytes to fill
 */
void *memset(void *dest, int val, size_t n);

/**
 * memcpy - Copy a memory area from source to destination
 * @dest: Pointer to the destination memory buffer
 * @src: Pointer to the source memory buffer (must not overlap with dest)
 * @n: Number of bytes to copy
 */
void *memcpy(void *dest, const void *src, size_t n);

/**
 * memmove - Copy memory area with overlap safety
 * @dest: Pointer to the destination memory buffer
 * @src: Pointer to the source memory buffer (may safely overlap with dest)
 * @n: Number of bytes to copy
 */
void *memmove(void *dest, const void *src, size_t n);

/**
 * memcmp - Compare two memory buffers byte-by-byte
 * @s1: Pointer to the first memory buffer
 * @s2: Pointer to the second memory buffer
 * @n: Number of bytes to compare
 */
int memcmp(const void *s1, const void *s2, size_t n);

/**
 * memchr - Find the first occurrence of a character byte in a memory area
 * @s: Pointer to the memory area to search
 * @c: Character byte to look for
 * @n: Number of bytes to inspect
 */
void *memchr(const void *s, int c, size_t n);

/*
 * String Operations (Null-terminated C strings)
 */

/**
 * strlen - Calculate the length of a null-terminated string
 * @str: Pointer to the null-terminated string
 */
size_t strlen(const char *str);

/**
 * strnlen - Calculate length of a string with a maximum limit
 * @str: Pointer to the string
 * @maxlen: Maximum number of characters to inspect
 */
size_t strnlen(const char *str, size_t maxlen);

/**
 * strcmp - Compare two null-terminated strings
 * @s1: Pointer to the first string
 * @s2: Pointer to the second string
 */
int strcmp(const char *s1, const char *s2);

/**
 * strncmp - Compare two strings up to a specified character count
 * @s1: Pointer to the first string
 * @s2: Pointer to the second string
 * @n: Maximum number of characters to compare
 */
int strncmp(const char *s1, const char *s2, size_t n);

/**
 * strcpy - Copy a null-terminated string to destination buffer
 * @dest: Pointer to destination buffer (must be large enough)
 * @src: Pointer to source string to copy
 */
char *strcpy(char *dest, const char *src);

/**
 * strncpy - Copy string with maximum length limit
 * @dest: Pointer to destination buffer
 * @src: Pointer to source string
 * @n: Maximum number of characters to copy
 */
char *strncpy(char *dest, const char *src, size_t n);

/**
 * strcat - Append one string to the end of another
 * @dest: Pointer to destination string (must have enough space)
 * @src: Pointer to source string to append
 */
char *strcat(char *dest, const char *src);

/**
 * strncat - Append up to n characters of a string to destination
 * @dest: Pointer to destination string
 * @src: Pointer to source string to append
 * @n: Maximum number of characters to append
 */
char *strncat(char *dest, const char *src, size_t n);

/**
 * strchr - Find first occurrence of a character in a string
 * @str: Pointer to null-terminated string to search
 * @c: Character to search for
 */
char *strchr(const char *str, int c);

/**
 * strrchr - Find last occurrence of a character in a string
 * @str: Pointer to null-terminated string to search
 * @c: Character to search for
 */
char *strrchr(const char *str, int c);

/**
 * strstr - Locate a substring within another string
 * @haystack: Pointer to string to search through
 * @needle: Pointer to substring to find
 */
char *strstr(const char *haystack, const char *needle);
