/*
 * kernel/include/lib/log.h
 *
 * PURPOSE:
 * Early kernel logging with colored status badges and subsystem scopes.
 *
 * USAGE:
 * 1. Define LOG_SCOPE at the top of your .c file before including this header:
 *
 *    #define LOG_SCOPE "BOOT"
 *    #include "lib/log.h"
 *
 * 2. Use the shorthand macros anywhere in that file:
 *
 *    pr_ok("Multiboot2 verified.");       // [ OK ] [BOOT] Multiboot2 verified.
 *    pr_info("Found 128 MB RAM.");        // [INFO] [BOOT] Found 128 MB RAM.
 *    pr_warn("ACPI table missing.");      // [WARN] [BOOT] ACPI table missing.
 *    pr_err("Out of memory!");            // [ERR ] [BOOT] Out of memory!
 *
 *    pr_err("Failed with code %d", err);  // [ERR ] [BOOT] Failed with code -5
 *
 * 3. Alternatively, call the functions directly with an explicit scope:
 *
 *    log_info("PMM", "Page allocator initialized.");
 */

#pragma once

// The message is a printf-style format (see lib/kprintf.h), truncated to 255 chars.
void log_ok(const char *scope, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void log_info(const char *scope, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void log_warn(const char *scope, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void log_error(const char *scope, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#ifndef LOG_SCOPE
#define LOG_SCOPE "KERNEL"
#endif

#define pr_ok(...) log_ok(LOG_SCOPE, __VA_ARGS__)
#define pr_info(...) log_info(LOG_SCOPE, __VA_ARGS__)
#define pr_warn(...) log_warn(LOG_SCOPE, __VA_ARGS__)
#define pr_err(...) log_error(LOG_SCOPE, __VA_ARGS__)
