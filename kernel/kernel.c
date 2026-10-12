void kernel_main(void) {
  volatile char *vga = (volatile char *)0xB8000;
  const char *msg = "Hello, DoomOS here!";

  for (int i = 0; msg[i] != '\0'; i++) {
    vga[i * 2] = msg[i];   // The character byte
    vga[i * 2 + 1] = 0x0F; // White color byte
  }

  while (1) {
    __asm__ volatile("hlt");
  }
}
