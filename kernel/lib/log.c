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

void log_info(const char *scope, const char *msg)
{
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write("[INFO] ");
    print_scope(scope);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write(msg);
    vga_write("\n");
}

void log_warn(const char *scope, const char *msg)
{
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_write("[WARN] ");
    print_scope(scope);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write(msg);
    vga_write("\n");
}

void log_error(const char *scope, const char *msg)
{
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_write("[ERR ] ");
    print_scope(scope);
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_write(msg);
    vga_write("\n");
}

void log_ok(const char *scope, const char *msg)
{
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_write("[ OK ] ");
    print_scope(scope);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write(msg);
    vga_write("\n");
}
