#include <stdarg.h>

#include "lib/kprintf.h"
#include "lib/log.h"
#include "vga.h"

static void print_scope(const char *scope)
{
    if (scope && *scope != '\0') {
        vga_set_color(VGA_COLOR_LIGHT_GRAY, VGA_COLOR_BLACK);
        vga_write("[");
        vga_write(scope);
        vga_write("] ");
    }
}

static void log_emit(u8 badge_color, const char *badge, u8 msg_color, const char *scope,
                     const char *fmt, va_list args)
{
    // shortcut: messages over 255 chars are truncated, same as kprintf
    char buf[256];
    vsnprintf(buf, sizeof buf, fmt, args);

    vga_set_color(badge_color, VGA_COLOR_BLACK);
    vga_write(badge);
    print_scope(scope);
    vga_set_color(msg_color, VGA_COLOR_BLACK);
    vga_write(buf);
    vga_write("\n");
}

void log_info(const char *scope, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_emit(VGA_COLOR_LIGHT_CYAN, "[INFO] ", VGA_COLOR_WHITE, scope, fmt, args);
    va_end(args);
}

void log_warn(const char *scope, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_emit(VGA_COLOR_YELLOW, "[WARN] ", VGA_COLOR_WHITE, scope, fmt, args);
    va_end(args);
}

void log_error(const char *scope, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_emit(VGA_COLOR_LIGHT_RED, "[ERR ] ", VGA_COLOR_LIGHT_RED, scope, fmt, args);
    va_end(args);
}

void log_ok(const char *scope, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_emit(VGA_COLOR_LIGHT_GREEN, "[ OK ] ", VGA_COLOR_WHITE, scope, fmt, args);
    va_end(args);
}
