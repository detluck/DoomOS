CC = gcc
LD = ld
AS = nasm

# Compiler flags for freestanding 64-bit kernel
CFLAGS = -m64 -ffreestanding -fno-stack-protector -mno-red-zone -Wall -Wextra -nostdlib -O2 -Ikernel/include
ASFLAGS = -f elf64
LDFLAGS = -n -T linker.ld --no-warn-rwx-segments

BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/isodir

# Source files
ASM_SRC = boot/header.asm boot/boot.asm
C_SRC = $(shell find kernel -type f -name '*.c')

# Object files
ASM_OBJ = $(patsubst boot/%.asm, $(BUILD_DIR)/%.o, $(ASM_SRC))
C_OBJ = $(patsubst kernel/%.c, $(BUILD_DIR)/%.o, $(C_SRC))

KERNEL_BIN = $(BUILD_DIR)/kernel.bin
KERNEL_BIN32 = $(BUILD_DIR)/kernel32.bin
KERNEL_ISO = $(BUILD_DIR)/kernel.iso

HOST_CC ?= gcc

.PHONY: all run run-iso iso clean test format format-check lint

all: $(KERNEL_BIN) $(KERNEL_BIN32)

# Assemble NASM files
$(BUILD_DIR)/%.o: boot/%.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Compile C files
$(BUILD_DIR)/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link 64-bit ELF kernel binary (for GRUB)
$(KERNEL_BIN): $(ASM_OBJ) $(C_OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

# 32-bit ELF wrapper (enables direct QEMU -kernel booting)
$(KERNEL_BIN32): $(KERNEL_BIN)
	objcopy -I elf64-x86-64 -O elf32-i386 $< $@

# Create bootable ISO with GRUB
iso: $(KERNEL_BIN)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/kernel.bin
	cp boot/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(KERNEL_ISO) $(ISO_DIR)

# Run directly in QEMU (Multiboot 1 direct kernel boot)
run: $(KERNEL_BIN32)
	qemu-system-x86_64 -kernel $(KERNEL_BIN32)

# Run GRUB bootable ISO in QEMU (Multiboot 2 via GRUB)
run-iso: iso
	qemu-system-x86_64 -cdrom $(KERNEL_ISO)

# Host-side unit tests
test:
	@mkdir -p $(BUILD_DIR)
	$(HOST_CC) -Wall -Wextra -fno-builtin -Ikernel/include tests/test_string.c kernel/lib/string.c -o $(BUILD_DIR)/test_string
	./$(BUILD_DIR)/test_string

# Code formatting (in-place)
format:
	find kernel tests -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +

# Code formatting check (dry-run for CI)
format-check:
	find kernel tests -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format --dry-run --Werror {} +

# Static analysis
lint:
	cppcheck --enable=warning,style,performance,portability \
	         --error-exitcode=1 \
	         --inline-suppr \
	         --suppress=missingIncludeSystem \
	         --suppress=unusedFunction \
	         -Ikernel/include \
	         kernel/

clean:
	rm -rf $(BUILD_DIR)
