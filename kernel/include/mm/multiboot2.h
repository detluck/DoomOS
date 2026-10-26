#pragma once

#include "types.h"

/* Multiboot2 bootloader magic passed in EAX */
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

/* Tag alignment requirement (8-byte boundary) */
#define MULTIBOOT_TAG_ALIGN 8

/* Multiboot2 Information Tag Types */
#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_CMDLINE 1

#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT_TAG_TYPE_MODULE 3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO 4
#define MULTIBOOT_TAG_TYPE_BOOTDEV 5
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_VBE 7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS 9
#define MULTIBOOT_TAG_TYPE_APM 10
#define MULTIBOOT_TAG_TYPE_EFI32 11
#define MULTIBOOT_TAG_TYPE_EFI64 12
#define MULTIBOOT_TAG_TYPE_SMBIOS 13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD 14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW 15
#define MULTIBOOT_TAG_TYPE_NETWORK 16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP 17
#define MULTIBOOT_TAG_TYPE_EFI_BS 18
#define MULTIBOOT_TAG_TYPE_EFI32_IH 19
#define MULTIBOOT_TAG_TYPE_EFI64_IH 20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR 21

/* Memory Map Region Types */
#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MULTIBOOT_MEMORY_RESERVED 2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS 4
#define MULTIBOOT_MEMORY_BADRAM 5

// Main Multiboot2 Information Header
struct multiboot_info {
    u32 total_size;
    u32 reserved;
};

// Generic Multiboot2 Tag Header
struct multiboot_tag {
    u32 type;
    u32 size;
};

// Basic Memory Information Tag
struct multiboot_tag_basic_meminfo {
    u32 type;
    u32 size;
    u32 mem_lower;
    u32 mem_upper;
};

// Boot Device Tag
struct multiboot_tag_bootdev {
    u32 type;
    u32 size;
    u32 biosdev;
    u32 slice;
    u32 part;
};

// Memory Map Entry
struct multiboot_mmap_entry {
    u64 addr;
    u64 len;
    u32 type;
    u32 zero;
};

// Memory Map Tag
struct multiboot_tag_mmap {
    u32 type;
    u32 size;
    u32 entry_size;
    u32 entry_version;
    struct multiboot_mmap_entry entries[];
};

bool multiboot_init(u64 magic, u64 info_adr);

const struct multiboot_tag_mmap *get_multiboot_mmap(void);

void print_multiboot_mmap(void);
