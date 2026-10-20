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
 * 3. Alternatively, call the functions directly with an explicit scope:
 *
 *    log_info("PMM", "Page allocator initialized.");
 */

#pragma once

void log_ok(const char *scope, const char *msg);
void log_info(const char *scope, const char *msg);
void log_warn(const char *scope, const char *msg);
void log_error(const char *scope, const char *msg);

#ifndef LOG_SCOPE
#define LOG_SCOPE "KERNEL"
#endif

#define pr_ok(msg) log_ok(LOG_SCOPE, msg)
#define pr_info(msg) log_info(LOG_SCOPE, msg)
#define pr_warn(msg) log_warn(LOG_SCOPE, msg)
#define pr_err(msg) log_error(LOG_SCOPE, msg)
