/* Security audit trail (ring buffer). Real cards also forward this to syslog/SIEM. */
#include "log.h"
#include "security.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define AUDIT_MAX 64
typedef struct {
    long ts;
    char who[32];
    char action[32];
    char result[48];
} audit_t;
static audit_t ring[AUDIT_MAX];
static int head, count;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void json_safe(char *dst, size_t cap, const char *src)
{
    size_t o = 0;
    for (; *src && o + 1 < cap; src++)
        dst[o++] = (char)((*src == '"' || *src == '\\' || (unsigned char)*src < 0x20) ? '_' : *src);
    dst[o] = 0;
}

void audit_log(const char *who, const char *action, const char *result)
{
    pthread_mutex_lock(&lock);
    audit_t *a = &ring[head];
    a->ts = (long)time(NULL);
    json_safe(a->who, sizeof a->who, who ? who : "-");
    json_safe(a->action, sizeof a->action, action);
    json_safe(a->result, sizeof a->result, result);
    head = (head + 1) % AUDIT_MAX;
    if (count < AUDIT_MAX) count++;
    pthread_mutex_unlock(&lock);
    nmc_log(LOG_INF, "audit", "who=%s action=%s result=%s", who ? who : "-", action, result);
}

int audit_dump_json(char *buf, size_t cap)
{
    pthread_mutex_lock(&lock);
    size_t o = (size_t)snprintf(buf, cap, "[");
    for (int i = 0; i < count && o < cap; i++) {
        audit_t *a = &ring[(head - count + i + AUDIT_MAX) % AUDIT_MAX];
        o += (size_t)snprintf(buf + o, cap - o, "%s{\"ts\":%ld,\"who\":\"%s\",\"action\":\"%s\",\"result\":\"%s\"}", i ? "," : "",
                              a->ts, a->who, a->action, a->result);
    }
    if (o < cap) o += (size_t)snprintf(buf + o, cap - o, "]");
    pthread_mutex_unlock(&lock);
    return o < cap ? (int)o : -1;
}
