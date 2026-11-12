#include "lib/kprintf.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// The malformed formats ("%q", trailing "%", "") are deliberate.
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-zero-length"

static void test_numbers(void)
{
    char buf[128];

    assert(snprintf(buf, sizeof(buf), "%d", 0) == 1);
    assert(strcmp(buf, "0") == 0);

    assert(snprintf(buf, sizeof(buf), "%d", -1) == 2);
    assert(strcmp(buf, "-1") == 0);

    assert(snprintf(buf, sizeof(buf), "%d", 2147483647) == 10);
    assert(strcmp(buf, "2147483647") == 0);

    assert(snprintf(buf, sizeof(buf), "%d", (-2147483647 - 1)) == 11);
    assert(strcmp(buf, "-2147483648") == 0);

    assert(snprintf(buf, sizeof(buf), "%ld", 9223372036854775807L) == 19);
    assert(strcmp(buf, "9223372036854775807") == 0);

    assert(snprintf(buf, sizeof(buf), "%ld", (-9223372036854775807L - 1)) == 20);
    assert(strcmp(buf, "-9223372036854775808") == 0);

    assert(snprintf(buf, sizeof(buf), "%u", 4294967295u) == 10);
    assert(strcmp(buf, "4294967295") == 0);

    assert(snprintf(buf, sizeof(buf), "%lu", 18446744073709551615UL) == 20);
    assert(strcmp(buf, "18446744073709551615") == 0);

    assert(snprintf(buf, sizeof(buf), "%llu", 18446744073709551615ULL) == 20);
    assert(strcmp(buf, "18446744073709551615") == 0);

    assert(snprintf(buf, sizeof(buf), "%x", 0xbeefu) == 4);
    assert(strcmp(buf, "beef") == 0);

    assert(snprintf(buf, sizeof(buf), "%lx", 0xffffffffffffffffUL) == 16);
    assert(strcmp(buf, "ffffffffffffffff") == 0);

    assert(snprintf(buf, sizeof(buf), "%b", 0u) == 1);
    assert(strcmp(buf, "0") == 0);

    assert(snprintf(buf, sizeof(buf), "%b", 5u) == 3);
    assert(strcmp(buf, "101") == 0);

    assert(snprintf(buf, sizeof(buf), "%lb", 0xffffffffffffffffUL) == 64);
    assert(strspn(buf, "1") == 64);

    printf("[PASS] numbers\n");
}

static void test_text(void)
{
    char buf[128];

    assert(snprintf(buf, sizeof(buf), "%s", "") == 0);
    assert(strcmp(buf, "") == 0);

    const char *null_str = NULL;
    assert(snprintf(buf, sizeof(buf), "%s", null_str) == 6);
    assert(strcmp(buf, "(null)") == 0);

    assert(snprintf(buf, sizeof(buf), "%s %c %%", "hi", 'A') == 6);
    assert(strcmp(buf, "hi A %") == 0);

    assert(snprintf(buf, sizeof(buf), "%q") == 2);
    assert(strcmp(buf, "%q") == 0);

    assert(snprintf(buf, sizeof(buf), "ab%") == 2);
    assert(strcmp(buf, "ab") == 0);

    assert(snprintf(buf, sizeof(buf), "") == 0);
    assert(strcmp(buf, "") == 0);

    printf("[PASS] text\n");
}

static void test_pointer(void)
{
    char buf[128];

    assert(snprintf(buf, sizeof(buf), "%p", (void *) NULL) == 5);
    assert(strcmp(buf, "(nil)") == 0);

    assert(snprintf(buf, sizeof(buf), "%p", (void *) 0x1234) == 6);
    assert(strcmp(buf, "0x1234") == 0);

    printf("[PASS] pointer\n");
}

static void test_truncation(void)
{
    char buf[8];

    assert(snprintf(buf, 5, "abcdefgh") == 8);
    assert(strcmp(buf, "abcd") == 0);

    memset(buf, 'X', sizeof(buf));
    assert(snprintf(buf, 1, "abc") == 3);
    assert(buf[0] == '\0');
    assert(buf[1] == 'X');

    assert(snprintf(NULL, 0, "%d", 12345) == 5);

    memset(buf, 'X', sizeof(buf));
    assert(snprintf(buf, 4, "%d", 12345) == 5);
    assert(strcmp(buf, "123") == 0);
    assert(buf[4] == 'X');

    printf("[PASS] truncation\n");
}

int main(void)
{
    printf("--- Running vsnprintf Unit Tests ---\n");

    test_numbers();
    test_text();
    test_pointer();
    test_truncation();

    printf("--- All 4 Unit Tests Passed! ---\n");
    return 0;
}
