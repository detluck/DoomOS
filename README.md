# x86-64 Kernel

The kernel of an operating system for 64-bit x86 processors that my team and I developed as part of the OSLab Betriebssystem project while studying at HAW Hamburg. The main goal is to run Doom on our kernel. More to come.

---

## Prerequisites & Dependencies

Install the required build toolchain, emulator, and packaging utilities:

```bash
sudo pacman -S gcc nasm binutils make qemu-system-x86 qemu-ui-gtk grub xorriso
```

### Dependency Breakdown

- **`gcc`**: C compiler (used with `-ffreestanding -m64` flags).
- **`nasm`**: Netwide Assembler for early 32-bit bootstrap and Long Mode initialization.
- **`binutils`**: Linker (`ld`) and binary tools (`objcopy`, `objdump`).
- **`make`**: GNU Make build automation.
- **`qemu-system-x86`**: x86-64 PC system emulator.
- **`qemu-ui-gtk`**: Graphical window display interface for QEMU (prevents headless VNC fallback).
- **`grub`**: GRUB 2 bootloader tools (`grub-mkrescue`).
- **`xorriso`**: ISO 9660 filesystem tool required by `grub-mkrescue` to create bootable ISOs.

---

## Quick Start

1. **Build and launch in QEMU**:

   ```bash
   make run
   ```

2. **Build a bootable ISO**:

   ```bash
   make iso
   ```
