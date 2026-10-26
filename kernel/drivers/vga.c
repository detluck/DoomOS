#include "vga.h"
#include "arch/io.h"

// https://github.com/torvalds/linux/blob/master/include/video/vga.h

static size_t cursor_row = 0;
static size_t cursor_col = 0;
static uint8_t color = 0;
static volatile uint16_t *buffer = VGA_MEMORY;

static inline uint16_t make_entry(char c, uint8_t color)
{
    return (uint16_t) ((uint16_t) color << 8) | (uint8_t) c;
}

// crtc registers on ports 0x3D4 / 0x3D5
/*static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}*/

/*static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}*/

static void update_hardware_cursor(void)
{
    uint16_t pos = (uint16_t) (cursor_row * VGA_WIDTH + cursor_col);
    outb(0x3D4, 14);
    outb(0x3D5, (uint8_t) (pos >> 8));
    outb(0x3D4, 15);
    outb(0x3D5, (uint8_t) (pos & 0xFF));
}

static uint16_t read_hardware_cursor(void)
{
    uint16_t pos;
    outb(0x3D4, 14);
    pos = (uint16_t) inb(0x3D5) << 8;
    outb(0x3D4, 15);
    pos |= inb(0x3D5);
    return pos;
}

void vga_enable_cursor(uint8_t start_scanline, uint8_t end_scanline)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, (uint8_t) ((inb(0x3D5) & 0xC0) | start_scanline));
    outb(0x3D4, 0x0B);
    outb(0x3D5, (uint8_t) ((inb(0x3D5) & 0xE0) | end_scanline));
}

void vga_set_color(uint8_t fg, uint8_t bg)
{
    color = (uint8_t) (fg | (bg << 4));
}

void vga_set_cursor_pos(size_t row, size_t col)
{
    cursor_row = row;
    cursor_col = col;
    update_hardware_cursor();
}

void vga_get_cursor_pos(size_t *row, size_t *col)
{
    uint16_t pos = read_hardware_cursor();
    *row = pos / VGA_WIDTH;
    *col = pos % VGA_WIDTH;
}

void vga_clear(void)
{
    color = 0;

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            buffer[y * VGA_WIDTH + x] = make_entry(' ', color);
        }
    }

    cursor_row = 0;
    cursor_col = 0;
    update_hardware_cursor();
}

void vga_scroll(void)
{
    // move every row up by one
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            buffer[(y - 1) * VGA_WIDTH + x] = buffer[y * VGA_WIDTH + x];
        }
    }

    // clear last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = make_entry(' ', color);
    }

    if (cursor_row > 0) {
        cursor_row--;
    }
}

static void newline(void)
{
    cursor_col = 0;
    if (++cursor_row == VGA_HEIGHT) {
        vga_scroll();
    }
}

void vga_putchar(char c)
{
    if (c == '\n') {
        newline();
    } else if (c == '\r') {
        cursor_col = 0;
    } else if (c == '\b') {
        vga_backspace();
        return;
    } else if (c == '\t') {
        // tab, multiples of 8
        do {
            buffer[cursor_row * VGA_WIDTH + cursor_col] = make_entry(' ', color);
        } while (++cursor_col % 8 != 0 && cursor_col < VGA_WIDTH);
    } else {
        buffer[cursor_row * VGA_WIDTH + cursor_col] = make_entry(c, color);
        if (++cursor_col == VGA_WIDTH) {
            newline();
        }
    }
    update_hardware_cursor();
}

void vga_backspace(void)
{
    if (cursor_col == 0) {
        if (cursor_row == 0) {
            return;
        }
        cursor_row--;
        cursor_col = VGA_WIDTH - 1;
    } else {
        cursor_col--;
    }
    buffer[cursor_row * VGA_WIDTH + cursor_col] = make_entry(' ', color);
    update_hardware_cursor();
}

void vga_write(const char *str)
{
    for (size_t i = 0; str[i] != '\0'; i++) {
        vga_putchar(str[i]);
    }
}

void vga_write_hex(uint64_t value)
{
    const char *digits = "0123456789ABCDEF";
    char tmp[17];
    int i = 16;
    tmp[i--] = '\0';
    if (value == 0) {
        vga_putchar('0');
        return;
    }
    while (value && i >= 0) {
        tmp[i--] = digits[value & 0xF];
        value >>= 4;
    }
    vga_write(&tmp[i + 1]);
}

void vga_set_blink(bool enabled)
{
    uint8_t value;

    inb(0x3DA);
    outb(0x3C0, 0x10);
    value = inb(0x3C1);

    if (enabled)
        value |= (1 << 3);
    else
        value &= ~(1 << 3);

    inb(0x3DA);
    outb(0x3C0, 0x10);
    outb(0x3C0, value);
    outb(0x3C0, 0x20);
}

void vga_init(void)
{
    vga_set_blink(false);

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_enable_cursor(0, 15); // full-height blinking cursor
    vga_clear();
}
