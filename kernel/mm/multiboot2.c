#include "mm/multiboot2.h"
#define LOG_SCOPE "MULTIBOOT"
#include "lib/log.h"

static const struct multiboot_tag_mmap *mmap_tag = NULL;

bool multiboot_init(u64 magic, u64 info_adr)
{
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        pr_err("Magic is not equal to multiboot2 specs");
        return false;
    }
    const struct multiboot_info *info = (const struct multiboot_info *) info_adr;

    if (info == NULL) {
        pr_err("Info strcut is NULL");
        return false;
    }
    if (info->total_size < 16) {
        pr_err("Size field in info strcut < 16");
        return false;
    }
    if (info->reserved != 0) {
        pr_err("Reserved field is not 0");
        return false;
    }
    const u8 *base = (const u8 *) info;
    const u8 *end = (const u8 *) base + info->total_size;
    u8 *ptr = (u8 *) base + 8;

    while (ptr <= end) {
        const struct multiboot_tag *tag = (const struct multiboot_tag *) ptr;
        if (tag->size == 0) {
            pr_err("Corrupt tag with size 0");
            break;
        }
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            mmap_tag = (const struct multiboot_tag_mmap *) tag;
        }
        if (tag->type == MULTIBOOT_TAG_TYPE_END) {
            break;
        }
        ptr += (tag->size + 7) & ~7;
    }
    return mmap_tag != NULL;
}
