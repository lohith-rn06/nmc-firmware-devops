#include "log.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static int g_level = LOG_INF;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static const char *names[] = {"DBG", "INF", "WRN", "ERR"};

void nmc_log_set_level(int level) { g_level = level; }

void nmc_log(int level, const char *module, const char *fmt, ...)
{
    if (level < g_level) return;
    char ts[32];
    time_t t = time(NULL);
    struct tm tm;
    gmtime_r(&t, &tm);
    strftime(ts, sizeof ts, "%Y-%m-%dT%H:%M:%SZ", &tm);
    char msg[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt,
              ap); // NOLINT(clang-analyzer-valist.Uninitialized): known analyzer false positive, ap is va_start-ed above
    va_end(ap);
    pthread_mutex_lock(&g_lock);
    fprintf(stderr, "%s [%s] %-7s %s\n", ts, names[level & 3], module, msg);
    pthread_mutex_unlock(&g_lock);
}
