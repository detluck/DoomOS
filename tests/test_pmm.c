#include "mm/pmm.h"
#include "types.h"
#include "lib/kprintf.h"
#include "lib/assert.h"

static void test_pmm_constants_and_macros(void)
{
    /* Page sizes and shifts */
    assert(PAGE_SHIFT == 12);
    assert(PAGE_SIZE == 4096);
    assert(PAGE_MASK == ~(4096UL - 1));

    /* PFN to Physical Address */
    assert(PFN_TO_PHYS(0) == 0x0);
    assert(PFN_TO_PHYS(1) == 0x1000);
    assert(PFN_TO_PHYS(2) == 0x2000);
    assert(PFN_TO_PHYS(256) == 0x100000);    /* 1 MiB */
    assert(PFN_TO_PHYS(32768) == 0x8000000); /* 128 MiB */

    /* Physical Address to PFN */
    assert(PHYS_TO_PFN(0x0) == 0);
    assert(PHYS_TO_PFN(0x1000) == 1);
    assert(PHYS_TO_PFN(0x1050) == 1); /* Truncates offset within frame */
    assert(PHYS_TO_PFN(0x1FFF) == 1);
    assert(PHYS_TO_PFN(0x2000) == 2);
    assert(PHYS_TO_PFN(0x100000) == 256);
    assert(PHYS_TO_PFN(0x8000000) == 32768);

    kprintf("[PASS] pmm_constants_and_macros\n");
}

static void test_bitmap_word_calculations(void)
{
    /* Test ceiling division for 64-bit words */
    assert((0 + 63) / 64 == 0);
    assert((1 + 63) / 64 == 1);
    assert((64 + 63) / 64 == 1);
    assert((65 + 63) / 64 == 2);
    assert((128 + 63) / 64 == 2);
    assert((129 + 63) / 64 == 3);

    /* 128 MiB RAM = 32768 frames -> exactly 512 u64 words = 4096 bytes (1 page) */
    uint64_t total_frames = 32768;
    uint64_t words = (total_frames + 63) / 64;
    uint64_t bytes = words * sizeof(uint64_t);
    assert(words == 512);
    assert(bytes == 4096);

    kprintf("[PASS] bitmap_word_calculations\n");
}

static void test_bitmap_bitwise_operations(void)
{
    uint64_t mock_bitmap[4] = {0};

    /* Helper macros mirroring pmm.c logic */
#define MOCK_SET(p) (mock_bitmap[(p) / 64] |= (1ULL << ((p) % 64)))
#define MOCK_CLEAR(p) (mock_bitmap[(p) / 64] &= ~(1ULL << ((p) % 64)))
#define MOCK_TEST(p) ((mock_bitmap[(p) / 64] & (1ULL << ((p) % 64))) != 0)

    /* Test bit 0 */
    assert(!MOCK_TEST(0));
    MOCK_SET(0);
    assert(MOCK_TEST(0));
    assert(mock_bitmap[0] == 1ULL);
    MOCK_CLEAR(0);
    assert(!MOCK_TEST(0));
    assert(mock_bitmap[0] == 0ULL);

    /* Test bit 63 (last bit of first word) */
    assert(!MOCK_TEST(63));
    MOCK_SET(63);
    assert(MOCK_TEST(63));
    assert(!MOCK_TEST(62));
    assert(!MOCK_TEST(64));
    MOCK_CLEAR(63);
    assert(!MOCK_TEST(63));

    /* Test bit 64 (first bit of second word) */
    assert(!MOCK_TEST(64));
    MOCK_SET(64);
    assert(MOCK_TEST(64));
    assert(mock_bitmap[0] == 0ULL);
    assert(mock_bitmap[1] == 1ULL);
    MOCK_CLEAR(64);
    assert(!MOCK_TEST(64));

    /* Fill first word and test scanning */
    mock_bitmap[0] = ~0ULL;
    assert(mock_bitmap[0] == 0xFFFFFFFFFFFFFFFFULL);
    for (int b = 0; b < 64; b++) {
        assert(MOCK_TEST(b));
    }

    /* Free single bit #13 in full word */
    MOCK_CLEAR(13);
    assert(!MOCK_TEST(13));
    assert(mock_bitmap[0] != ~0ULL);

    /* Scan finds bit 13 */
    int found_bit = -1;
    for (int b = 0; b < 64; b++) {
        if (!MOCK_TEST(b)) {
            found_bit = b;
            break;
        }
    }
    assert(found_bit == 13);

#undef MOCK_SET
#undef MOCK_CLEAR
#undef MOCK_TEST

    kprintf("[PASS] bitmap_bitwise_operations\n");
}

static void test_real_pmm_word_boundary(void)
{
    uint64_t initial_free = pmm_get_free_frames();

    // Allocate 65 frames.
    // This forces the PMM to cross from bit 63 (end of one u64)
    // to bit 64 (start of the next u64) in the real hardware bitmap!
    phys_addr_t block = pmm_alloc_frames(65);

    assert(block != 0);
    assert(pmm_get_free_frames() == initial_free - 65);

    // Free them, which tests if pmm_free_frames loops over the boundary correctly
    pmm_free_frames(block, 65);

    assert(pmm_get_free_frames() == initial_free);

    kprintf("[PASS] real_pmm_word_boundary\n");
}

void run_pmm_tests()
{
    kprintf("--- Running PMM Unit Tests ---\n");
    test_pmm_constants_and_macros();
    test_bitmap_word_calculations();
    test_bitmap_bitwise_operations();
    test_real_pmm_word_boundary();
    kprintf("--- All PMM Unit Tests Passed! ---\n");
}
