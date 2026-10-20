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
