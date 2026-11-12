CC = gcc
LD = ld
AS = nasm
# Compiler flags for freestanding 64-bit kernel
CFLAGS = -m64 -ffreestanding -fno-stack-protector -mno-red-zone -mgeneral-regs-only -Wall -Wextra -nostdlib -O2 -mcmodel=kernel -fno-pie -fno-pic -Ikernel/include
ASFLAGS = -f elf64
LDFLAGS = -n -T linker.ld --no-warn-rwx-segments -z max-page-size=0x1000

BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/isodir

# Recursively find files: $(call rwildcard,directory,pattern)
rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Source files
BOOT_ASM_SRC = boot/header.asm boot/boot.asm
ARCH_ASM_SRC = $(wildcard kernel/arch/*.asm)
C_SRC = $(call rwildcard,kernel,*.c)

# Object files
BOOT_ASM_OBJ = $(patsubst boot/%.asm, $(BUILD_DIR)/%.o, $(BOOT_ASM_SRC))
ARCH_ASM_OBJ = $(patsubst kernel/arch/%.asm, $(BUILD_DIR)/asm/arch/%.o, $(ARCH_ASM_SRC))
ASM_OBJ = $(BOOT_ASM_OBJ) $(ARCH_ASM_OBJ)
C_OBJ = $(patsubst kernel/%.c, $(BUILD_DIR)/%.o, $(C_SRC))

# Kernel integration tests
KERNEL_TEST_SRC = tests/test_pmm.c tests/test_heap.c
KERNEL_TEST_OBJ = $(patsubst tests/%.c, $(BUILD_DIR)/tests/%.o, $(KERNEL_TEST_SRC))

# Host unit tests
HOST_TESTS = string list vsnprintf
HOST_TEST_BIN = $(addprefix $(BUILD_DIR)/host_test_, $(HOST_TESTS))

ifeq ($(RUN_TESTS),1)
CFLAGS += -DRUN_TESTS
C_OBJ += $(KERNEL_TEST_OBJ)
endif

KERNEL_BIN = $(BUILD_DIR)/kernel.bin
KERNEL_ISO = $(BUILD_DIR)/kernel.iso

HOST_CC ?= gcc

.PHONY: all run run-iso iso clean test host-test format format-check lint

all: $(KERNEL_BIN)

# Assemble NASM files
$(BUILD_DIR)/%.o: boot/%.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Assemble kernel architecture NASM files separately from C objects
$(BUILD_DIR)/asm/arch/%.o: kernel/arch/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Compile C files
$(BUILD_DIR)/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link 64-bit ELF kernel binary
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

# Host unit tests
HOST_TESTS = string list vsnprintf
HOST_TEST_BIN = $(addprefix $(BUILD_DIR)/host_test_, $(HOST_TESTS))

HOST_CC ?= gcc
HOST_TEST_CFLAGS = -Wall -Wextra -fno-builtin -Ikernel/include \
                   -Wno-memset-transposed-args \
                   -Wno-stringop-truncation \
                   -Wno-stringop-overread \
                   -Wno-format-truncation \
                   -Wno-format

# String tests: link the actual kernel string implementation
$(BUILD_DIR)/host_test_string: tests/test_string.c kernel/lib/string.c
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_TEST_CFLAGS) $^ -o $@

# List tests: header-only implementation
$(BUILD_DIR)/host_test_list: tests/test_list.c
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_TEST_CFLAGS) $^ -o $@

# Formatter tests: link the actual kernel vsnprintf implementation
$(BUILD_DIR)/host_test_vsnprintf: tests/test_vsnprintf.c kernel/lib/vsnprintf.c
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_TEST_CFLAGS) $^ -o $@

# Compile kernel integration tests
$(BUILD_DIR)/tests/%.o: tests/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Run all host tests
host-test: $(HOST_TEST_BIN)
	@for test in $(HOST_TEST_BIN); do \
		printf "\nRunning %s...\n" "$$test"; \
		if ./$$test; then \
			printf '\033[0;32m[PASS] %s\033[0m\n' "$$test"; \
		else \
			status=$$?; \
			printf '\033[0;31m[FAIL] %s (exit code: %d)\033[0m\n' "$$test" "$$status"; \
			exit $$status; \
		fi; \
	done
	@printf '\n\033[0;32mAll host tests passed!\033[0m\n'

test:
	@echo "\n Building kernel level tests"
	$(MAKE) host-test

	$(MAKE) clean
	$(MAKE) iso RUN_TESTS=1

	@echo "\nStarting QEMU..."
	qemu-system-x86_64 -cdrom $(KERNEL_ISO)

# Code formatting (in-place)
format:
	find kernel tests -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +

# Code formatting check (dry-run for CI)
format-check:
	find kernel tests -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format --dry-run --Werror {} +

# Static analysis
lint:
	cppcheck --enable=all \
	         --inconclusive \
	         --check-level=exhaustive \
	         --std=c11 \
	         --error-exitcode=1 \
	         --inline-suppr \
	         --suppress=missingIncludeSystem \
	         --suppress=unusedFunction \
	         --suppress=staticFunction \
	         --suppress=checkersReport \
	         --suppress=unmatchedSuppression \
	         -Ikernel/include \
	         kernel/

clean:
	rm -rf $(BUILD_DIR)
