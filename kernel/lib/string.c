#include "lib/string.h"
#include "types.h"
#include <stddef.h>

void *memset(void *dest, int val, size_t n) {
  u8 *d = (u8 *)dest;

  while (n--) {
    *d++ = (u8)val;
  }
  return dest;
}

void *memcpy(void *dest, const void *src, size_t n) {
  u8 *d = (u8 *)dest;
  const u8 *s = (const u8 *)src;

  while (n--) {
    *d++ = *s++;
  }
  return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
  u8 *d = (u8 *)dest;
  const u8 *s = (const u8 *)src;

  if (d < s) {
    while (n--) {
      *d++ = *s++;
    }
  } else if (d > s) {
    d += n;
    s += n;
    while (n--) {
      *--d = *--s;
    }
  }
  return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const u8 *st1 = (const u8 *)s1;
  const u8 *st2 = (const u8 *)s2;

  while (n--) {
    if (*st1 != *st2) {
      return *st1 - *st2;
    }
    st1++;
    st2++;
  }
  return 0;
}

void *memchr(const void *s, int c, size_t n) {
  const u8 *st = (const u8 *)s;

  while (n--) {
    if ((u8)c == *st) {
      return (void *)st;
    }
    st++;
  }
  return NULL;
}

size_t strlen(const char *str) {
  const char *p = str;
  while (*p != '\0') {
    p++;
  }
  return p - str;
}

size_t strnlen(const char *str, size_t maxlen) {
  const char *p = str;
  while (maxlen--) {
    if (*p == '\0') {
      break;
    }
    p++;
  }
  return p - str;
}

int strcmp(const char *s1, const char *s2) {
  unsigned char st1, st2;
  while (1) {
    st1 = *s1++;
    st2 = *s2++;

    if (st1 != st2) {
      return st1 < st2 ? -1 : 1;
    }
    if (!st1) {
      break;
    }
  }
  return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  unsigned char st1, st2;
  while (n--) {
    st1 = *s1++;
    st2 = *s2++;

    if (st1 != st2) {
      return st1 < st2 ? -1 : 1;
    }
    if (!st1) {
      break;
    }
  }
  return 0;
}

char *strcpy(char *dest, const char *src) {
  char *p = dest;
  while ((*p++ = *src++) != '\0') {
    // nothing
  }
  return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
  char *d = dest;
  while (n > 0 && *src != '\0') {
    *d++ = *src++;
    n--;
  }
  while (n > 0) {
    *d++ = '\0';
    n--;
  }
  return dest;
}

char *strcat(char *dest, const char *src) {
  char *p = dest;
  while (*dest) {
    dest++;
  }
  while ((*dest++ = *src++) != '\0') {
    // nothing
  }
  return p;
}

char *strncat(char *dest, const char *src, size_t n) {
  char *d = dest;
  while (*d) {
    d++;
  }
  while (n > 0 && *src != '\0') {
    *d++ = *src++;
    n--;
  }
  *d = '\0';
  return dest;
}

char *strchr(const char *str, int c) {
  for (;; str++) {
    if (*str == (char)c) {
      return (char *)str;
    }
    if (*str == '\0') {
      return NULL;
    }
  }
}

char *strrchr(const char *str, int c) {
  const char *last = NULL;
  for (;; str++) {
    if (*str == (char)c) {
      last = str;
    }
    if (*str == '\0') {
      break;
    }
  }
  return (char *)last;
}

char *strstr(const char *haystack, const char *needle) {
  size_t l1, l2;
  l2 = strlen(needle);
  if (!l2) {
    return (char *)haystack;
  }
  l1 = strlen(haystack);
  while (l1 >= l2) {
    l1--;
    if (!memcmp(haystack, needle, l2)) {
      return (char *)haystack;
    }
    haystack++;
  }
  return NULL;
}
