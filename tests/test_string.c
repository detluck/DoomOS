#include "lib/string.h"
#include <assert.h>
#include <stdio.h>

static void test_memset(void)
{
    char buf[16];
    for (int i = 0; i < 16; i++) {
        buf[i] = 0x55;
    }

    void *ret = memset(buf, 0xAA, 10);
    assert(ret == buf);
    for (int i = 0; i < 10; i++) {
        assert((unsigned char) buf[i] == 0xAA);
    }
    for (int i = 10; i < 16; i++) {
        assert(buf[i] == 0x55);
    }

    // Edge case: n = 0
    ret = memset(buf, 0xFF, 0);
    assert(ret == buf);
    assert((unsigned char) buf[0] == 0xAA);
    printf("[PASS] memset\n");
}

static void test_memcpy(void)
{
    const char src[8] = "abcdefg";
    char dest[8] = {0};

    void *ret = memcpy(dest, src, 8);
    assert(ret == dest);
    for (int i = 0; i < 8; i++) {
        assert(dest[i] == src[i]);
    }

    // Edge case: n = 0
    char unchanged[4] = "xyz";
    ret = memcpy(unchanged, "abc", 0);
    assert(ret == unchanged);
    assert(unchanged[0] == 'x');
    printf("[PASS] memcpy\n");
}

static void test_memmove(void)
{
    // Overlap: dest > src (shift right)
    char buf1[16] = "0123456789";
    void *ret = memmove(buf1 + 2, buf1, 6);
    assert(ret == buf1 + 2);
    // Expected: "0101234589"
    assert(buf1[0] == '0' && buf1[1] == '1');
    assert(buf1[2] == '0' && buf1[3] == '1' && buf1[4] == '2');
    assert(buf1[5] == '3' && buf1[6] == '4' && buf1[7] == '5');

    // Overlap: dest < src (shift left)
    char buf2[16] = "0123456789";
    ret = memmove(buf2, buf2 + 2, 6);
    assert(ret == buf2);
    // Expected: "2345676789"
    assert(buf2[0] == '2' && buf2[1] == '3' && buf2[2] == '4');
    assert(buf2[3] == '5' && buf2[4] == '6' && buf2[5] == '7');

    // Edge case: dest == src
    ret = memmove(buf2, buf2, 5);
    assert(ret == buf2);

    // Edge case: n = 0
    ret = memmove(buf2, buf2 + 1, 0);
    assert(ret == buf2);
    printf("[PASS] memmove\n");
}

static void test_memcmp(void)
{
    char b1[] = "abcdef";
    char b2[] = "abcdef";
    char b3[] = "abcdeg";
    char b4[] = "abcdea";

    assert(memcmp(b1, b2, 6) == 0);
    assert(memcmp(b1, b3, 6) < 0);
    assert(memcmp(b1, b4, 6) > 0);
    assert(memcmp(b1, b3, 5) == 0);
    assert(memcmp(b1, b3, 0) == 0);

    // Unsigned char signedness check (0x80 should be > 0x01)
    unsigned char u1[] = {0x80, 0x00};
    unsigned char u2[] = {0x01, 0x00};
    assert(memcmp(u1, u2, 2) > 0);
    printf("[PASS] memcmp\n");
}

static void test_memchr(void)
{
    char buf[] = "abcdef";
    void *res = memchr(buf, 'c', 6);
    assert(res == buf + 2);

    res = memchr(buf, 'z', 6);
    assert(res == NULL);

    res = memchr(buf, 'c', 2);
    assert(res == NULL);

    res = memchr(buf, 'a', 0);
    assert(res == NULL);
    printf("[PASS] memchr\n");
}

static void test_strlen(void)
{
    assert(strlen("") == 0);
    assert(strlen("a") == 1);
    assert(strlen("DoomOS") == 6);
    assert(strlen("Hello, World!") == 13);
    printf("[PASS] strlen\n");
}

static void test_strnlen(void)
{
    assert(strnlen("", 10) == 0);
    assert(strnlen("DoomOS", 10) == 6);
    assert(strnlen("DoomOS", 3) == 3);
    assert(strnlen("DoomOS", 0) == 0);
    assert(strnlen("DoomOS", 6) == 6);
    printf("[PASS] strnlen\n");
}

static void test_strcmp(void)
{
    assert(strcmp("abc", "abc") == 0);
    assert(strcmp("abc", "abd") < 0);
    assert(strcmp("abd", "abc") > 0);
    assert(strcmp("abc", "abcd") < 0);
    assert(strcmp("abcd", "abc") > 0);
    assert(strcmp("", "") == 0);
    assert(strcmp("", "a") < 0);

    // Unsigned char test: 0x80 > 'A' (65)
    assert(strcmp("\x80", "A") > 0);
    printf("[PASS] strcmp\n");
}

static void test_strncmp(void)
{
    assert(strncmp("abc", "abd", 2) == 0);
    assert(strncmp("abc", "abd", 3) < 0);
    assert(strncmp("abd", "abc", 3) > 0);
    assert(strncmp("abc", "abc", 10) == 0);
    assert(strncmp("abc", "xyz", 0) == 0);
    printf("[PASS] strncmp\n");
}

static void test_strcpy(void)
{
    char dest[16];
    char *ret = strcpy(dest, "DoomOS");
    assert(ret == dest);
    assert(strcmp(dest, "DoomOS") == 0);

    ret = strcpy(dest, "");
    assert(ret == dest);
    assert(strcmp(dest, "") == 0);
    printf("[PASS] strcpy\n");
}

static void test_strncpy(void)
{
    char dest[10];

    // Truncation without null terminator if src >= n
    char *ret = strncpy(dest, "1234567890", 5);
    assert(ret == dest);
    assert(dest[0] == '1' && dest[4] == '5');

    // Padding with zeroes if src < n
    for (int i = 0; i < 10; i++) {
        dest[i] = 'X';
    }
    ret = strncpy(dest, "hi", 6);
    assert(ret == dest);
    assert(dest[0] == 'h' && dest[1] == 'i');
    for (int i = 2; i < 6; i++) {
        assert(dest[i] == '\0');
    }
    assert(dest[6] == 'X'); // Untouched past n

    // Edge case: n = 0
    dest[0] = 'K';
    ret = strncpy(dest, "hello", 0);
    assert(ret == dest);
    assert(dest[0] == 'K');
    printf("[PASS] strncpy\n");
}

static void test_strcat(void)
{
    char dest[32] = "Hello";
    char *ret = strcat(dest, ", World!");
    assert(ret == dest);
    assert(strcmp(dest, "Hello, World!") == 0);

    ret = strcat(dest, "");
    assert(ret == dest);
    assert(strcmp(dest, "Hello, World!") == 0);
    printf("[PASS] strcat\n");
}

static void test_strncat(void)
{
    char dest[32] = "Hello";
    char *ret = strncat(dest, ", World!", 3);
    assert(ret == dest);
    assert(strcmp(dest, "Hello, W") == 0);

    // Must always null terminate even if n is 0
    ret = strncat(dest, "XYZ", 0);
    assert(ret == dest);
    assert(strcmp(dest, "Hello, W") == 0);
    printf("[PASS] strncat\n");
}

static void test_strchr(void)
{
    const char *str = "Hello World";
    assert(strchr(str, 'W') == str + 6);
    assert(strchr(str, 'z') == NULL);

    // Critical standard compliance: searching for '\0' must return pointer to terminator
    char *term = strchr(str, '\0');
    assert(term == str + 11);
    assert(*term == '\0');
    printf("[PASS] strchr\n");
}

static void test_strrchr(void)
{
    const char *str = "banana";
    assert(strrchr(str, 'a') == str + 5);
    assert(strrchr(str, 'b') == str);
    assert(strrchr(str, 'z') == NULL);

    // Critical standard compliance: searching for '\0'
    char *term = strrchr(str, '\0');
    assert(term == str + 6);
    assert(*term == '\0');
    printf("[PASS] strrchr\n");
}

static void test_strstr(void)
{
    const char *haystack = "DoomOS Kernel v1.0";
    assert(strstr(haystack, "Kernel") == haystack + 7);
    assert(strstr(haystack, "Doom") == haystack);
    assert(strstr(haystack, "1.0") == haystack + 15);
    assert(strstr(haystack, "Quake") == NULL);
    assert(strstr(haystack, "v1.0-extended") == NULL);

    // Empty needle returns haystack
    assert(strstr(haystack, "") == haystack);
    printf("[PASS] strstr\n");
}

int main(void)
{
    printf("--- Running String Library Unit Tests ---\n");
    test_memset();
    test_memcpy();
    test_memmove();
    test_memcmp();
    test_memchr();
    test_strlen();
    test_strnlen();
    test_strcmp();
    test_strncmp();
    test_strcpy();
    test_strncpy();
    test_strcat();
    test_strncat();
    test_strchr();
    test_strrchr();
    test_strstr();
    printf("--- All 16 Unit Tests Passed! ---\n");
    return 0;
}
