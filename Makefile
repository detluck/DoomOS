CC = gcc
LD = ld
AS = nasm

# Compiler flags for freestanding 64-bit kernel
CFLAGS = -m64 -ffreestanding -fno-stack-protector -mno-red-zone -mgeneral-regs-only -Wall -Wextra -nostdlib -O2 -Ikernel/include
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
KERNEL_ISO = $(BUILD_DIR)/kernel.iso

HOST_CC ?= gcc

.PHONY: all run run-iso iso clean test format format-check lint

all: $(KERNEL_BIN)

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

# Create bootable ISO with GRUB
iso: $(KERNEL_BIN)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/kernel.bin
	cp boot/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(KERNEL_ISO) $(ISO_DIR)

# Run GRUB bootable ISO in QEMU (Multiboot 2)
run: iso
	qemu-system-x86_64 -cdrom $(KERNEL_ISO)

run-iso: run

# Host-side unit tests
test:
	@mkdir -p $(BUILD_DIR)
	$(HOST_CC) -Wall -Wextra -fno-builtin -Ikernel/include tests/test_string.c kernel/lib/string.c -o $(BUILD_DIR)/test_string
	./$(BUILD_DIR)/test_string
	$(HOST_CC) -Wall -Wextra -Ikernel/include tests/test_pmm.c -o $(BUILD_DIR)/test_pmm
	./$(BUILD_DIR)/test_pmm

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
