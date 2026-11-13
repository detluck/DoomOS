#include <stdarg.h>

#include "lib/kprintf.h"
#include "types.h"

// Bounded output cursor: counts everything, stores only what fits (keeping room for the NUL).
struct out {
    char *buf;
    size_t size;
    size_t pos;
};

static void emit(struct out *o, char c)
{
    if (o->pos + 1 < o->size) {
        o->buf[o->pos] = c;
    }
    o->pos++;
}

static void emit_str(struct out *o, const char *s)
{
    while (*s != '\0') {
        emit(o, *s++);
    }
}

static void print_num(struct out *o, u64 value, unsigned base)
{
    const char *digits = "0123456789abcdef";
    char buf[64];
    int i = 0;

    do {
        buf[i++] = digits[value % base];
        value /= base;
    } while (value != 0);

    while (i > 0) {
        emit(o, buf[--i]);
    }
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    struct out o = {buf, size, 0};

    for (; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            emit(&o, *fmt);
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
            emit(&o, (char) va_arg(args, int));
            break;
        case 's': {
            const char *str = va_arg(args, const char *);
            emit_str(&o, str ? str : "(null)");
            break;
        }
        case 'd': {
            i64 value = is_long ? va_arg(args, i64) : va_arg(args, int);
            if (value < 0) {
                emit(&o, '-');
                print_num(&o, -(u64) value, 10);
            } else {
                print_num(&o, (u64) value, 10);
            }
            break;
        }
        case 'u':
            print_num(&o, is_long ? va_arg(args, u64) : va_arg(args, unsigned int), 10);
            break;
        case 'x':
            print_num(&o, is_long ? va_arg(args, u64) : va_arg(args, unsigned int), 16);
            break;
        case 'b':
            print_num(&o, is_long ? va_arg(args, u64) : va_arg(args, unsigned int), 2);
            break;
        case 'p': {
            const void *ptr = va_arg(args, void *);
            if (ptr == NULL) {
                emit_str(&o, "(nil)");
            } else {
                emit_str(&o, "0x");
                print_num(&o, (u64) (uintptr_t) ptr, 16);
            }
            break;
        }
        case '%':
            emit(&o, '%');
            break;
        case '\0':
            goto done; // lone trailing '%'
        default:
            emit(&o, '%');
            emit(&o, *fmt);
            break;
        }
    }

done:
    if (size > 0) {
        buf[o.pos < size ? o.pos : size - 1] = '\0';
    }
    return (int) o.pos;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return n;
}
