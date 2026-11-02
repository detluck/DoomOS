#include "lib/kprintf.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// the malformed formats ("%q", trailing "%", "") are deliberate
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-zero-length"

#define CHECK(expected, ...) \
    do { \
        char b[128]; \
        int n = snprintf(b, sizeof b, __VA_ARGS__); \
        assert(strcmp(b, expected) == 0); \
        assert((size_t) n == strlen(expected)); \
    } while (0)

static void test_numbers(void)
{
    CHECK("0", "%d", 0);
    CHECK("-1", "%d", -1);
    CHECK("2147483647", "%d", 2147483647);
    CHECK("-2147483648", "%d", (-2147483647 - 1));
    CHECK("9223372036854775807", "%ld", 9223372036854775807L);
    CHECK("-9223372036854775808", "%ld", (-9223372036854775807L - 1));
    CHECK("0", "%u", 0u);
    CHECK("4294967295", "%u", 4294967295u);
    CHECK("18446744073709551615", "%lu", 18446744073709551615UL);
    CHECK("18446744073709551615", "%llu", 18446744073709551615ULL);
    CHECK("0", "%x", 0u);
    CHECK("beef", "%x", 0xbeefu);
    CHECK("ffffffff", "%x", 0xffffffffu);
    CHECK("ffffffffffffffff", "%lx", 0xffffffffffffffffUL);

    // %b is not known to the host's printf, so check it directly
    char b[80];
    assert(snprintf(b, sizeof b, "%b", 0u) == 1 && strcmp(b, "0") == 0);
    assert(snprintf(b, sizeof b, "%b", 5u) == 3 && strcmp(b, "101") == 0);
    assert(snprintf(b, sizeof b, "%lb", 0xffffffffffffffffUL) == 64);
    assert(strspn(b, "1") == 64);
}

static void test_text(void)
{
    CHECK("", "%s", "");
    const char *null_str = NULL; // via a variable so the compiler doesn't warn about a literal NULL
    CHECK("(null)", "%s", null_str);
    CHECK("hi A %", "%s %c %%", "hi", 'A');
    CHECK("%q", "%q");
    CHECK("ab", "ab%"); // lone trailing '%' stops cleanly
    CHECK("", "");
}

static void test_pointer(void)
{
    CHECK("(nil)", "%p", (void *) NULL);
    CHECK("0x1234", "%p", (void *) 0x1234);
}

static void test_truncation(void)
{
    char b[8];

    assert(snprintf(b, 5, "abcdefgh") == 8);
    assert(strcmp(b, "abcd") == 0);

    memset(b, 'X', sizeof b);
    assert(snprintf(b, 1, "abc") == 3);
    assert(b[0] == '\0' && b[1] == 'X');

    assert(snprintf(NULL, 0, "%d", 12345) == 5);

    // number cut mid-digits, canary after the buffer stays intact
    memset(b, 'X', sizeof b);
    assert(snprintf(b, 4, "%d", 12345) == 5);
    assert(strcmp(b, "123") == 0 && b[4] == 'X');
}

int main(void)
{
    test_numbers();
    printf("[PASS] numbers\n");
    test_text();
    printf("[PASS] text\n");
    test_pointer();
    printf("[PASS] pointer\n");
    test_truncation();
    printf("[PASS] truncation\n");
    return 0;
}
