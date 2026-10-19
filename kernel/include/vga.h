#ifndef VGA_H
#define VGA_H

#include <stddef.h>
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t *)0xB8000)

// https://en.wikipedia.org/wiki/VGA_text_mode
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GRAY = 7,
    VGA_COLOR_DARK_GRAY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
};

void vga_init(void);

void vga_putchar(char c);
void vga_write(const char *str);
void vga_write_hex(uint64_t value);

void vga_clear(void);
void vga_set_color(uint8_t fg, uint8_t bg);
void vga_enable_cursor(uint8_t start_scanline, uint8_t end_scanline);

void vga_set_cursor_pos(size_t row, size_t col);
void vga_get_cursor_pos(size_t *row, size_t *col);
void vga_scroll(void);

void vga_backspace(void);

#endif /* VGA_H */
