#include <stdarg.h>

#include "lib/kprintf.h"
#include "types.h"
#include "vga.h"

static void print_num(u64 value, unsigned base)
{
    const char *digits = "0123456789abcdef";
    char buf[20]; // u64 max is 20 decimal digits
    int i = 0;

    do {
        buf[i++] = digits[value % base];
        value /= base;
    } while (value != 0);

    while (i > 0) {
        vga_putchar(buf[--i]);
    }
}

void kprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    for (; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            vga_putchar(*fmt);
            continue;
        }
        fmt++;

        // 'l' or 'll' means a 64-bit argument
        bool is_long = false;
        while (*fmt == 'l') {
            is_long = true;
            fmt++;
        }

        switch (*fmt) {
        case 'c':
            vga_putchar((char) va_arg(args, int));
            break;
        case 's': {
            const char *str = va_arg(args, const char *);
            vga_write(str ? str : "(null)");
            break;
        }
        case 'd': {
            i64 value = is_long ? va_arg(args, i64) : va_arg(args, int);
            if (value < 0) {
                vga_putchar('-');
                print_num(-(u64) value, 10);
            } else {
                print_num(value, 10);
            }
            break;
        }
        case 'u':
            print_num(is_long ? va_arg(args, u64) : va_arg(args, unsigned int), 10);
            break;
        case 'x':
            print_num(is_long ? va_arg(args, u64) : va_arg(args, unsigned int), 16);
            break;
        case 'p':
            vga_write("0x");
            print_num((u64) va_arg(args, void *), 16);
            break;
        case '%':
            vga_putchar('%');
            break;
        case '\0':
            va_end(args);
            return;
        default:
            vga_putchar('%');
            vga_putchar(*fmt);
            break;
        }
    }

    va_end(args);
}
