#include <stdarg.h>

#include "lib/kprintf.h"
#include "vga.h"

void kprintf(const char *fmt, ...)
{
    // shortcut: output over 255 chars is truncated, raise the size if a caller needs more
    char buf[256];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);

    vga_write(buf);
}
