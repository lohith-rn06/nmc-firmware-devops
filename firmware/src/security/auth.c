/* Local user authentication with salted hash, constant-time compare and account lockout
 * (brute-force protection). RADIUS/LDAP would plug in here on real cards. */
#include "nmc.h"
#include "security.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int fails;
static long locked_until;

void auth_hash_password(const char *salt, const char *pass, char out[65])
{
    char buf[160];
    int n = snprintf(buf, sizeof buf, "%s%s", salt, pass);
    if (n < 0) n = 0;
    if ((size_t)n >= sizeof buf) n = sizeof buf - 1;
    sha256_hex((const uint8_t *)buf, (size_t)n, out);
    memset(buf, 0, sizeof buf);
}

static int ct_equal(const char *a, const char *b, size_t n)
{
    unsigned char r = 0;
    for (size_t i = 0; i < n; i++)
        r |= (unsigned char)(a[i] ^ b[i]);
    return r == 0;
}

void auth_reset(void)
{
    pthread_mutex_lock(&lock);
    fails = 0;
    locked_until = 0;
    pthread_mutex_unlock(&lock);
}

int auth_check(const char *user, const char *pass, long now)
{
    int rc;
    char h[65];
    pthread_mutex_lock(&lock);
    if (now < locked_until) {
        pthread_mutex_unlock(&lock);
        audit_log(user, "login", "locked");
        return AUTH_LOCKED;
    }
    auth_hash_password(g_cfg.salt, pass, h);
    int ok = ct_equal(h, g_cfg.admin_pass_hash, 64) & (strcmp(user, g_cfg.admin_user) == 0);
    if (ok) {
        fails = 0;
        rc = AUTH_OK;
    } else if (++fails >= AUTH_MAX_FAILS) {
        locked_until = now + AUTH_LOCK_SECONDS;
        fails = 0;
        rc = AUTH_LOCKED;
    } else
        rc = AUTH_FAIL;
    pthread_mutex_unlock(&lock);
    audit_log(user, "login", rc == AUTH_OK ? "success" : rc == AUTH_LOCKED ? "locked" : "failure");
    return rc;
}

int base64_decode(const char *in, char *out, size_t cap)
{
    static const char *tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t o = 0;
    uint32_t acc = 0;
    int bits = 0;
    for (; *in && *in != '='; in++) {
        const char *p = strchr(tbl, *in);
        if (!p || !*in) return -1;
        acc = (acc << 6) | (uint32_t)(p - tbl);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (o + 1 >= cap) return -1;
            out[o++] = (char)((acc >> bits) & 0xff);
        }
    }
    out[o] = 0;
    return (int)o;
}
