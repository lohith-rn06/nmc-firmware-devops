#ifndef NMC_SECURITY_H
#define NMC_SECURITY_H
#include <stddef.h>
#include <stdint.h>

void sha256(const uint8_t *data, size_t len, uint8_t out[32]);
void sha256_hex(const uint8_t *data, size_t len, char out[65]);

#define AUTH_OK 0
#define AUTH_FAIL (-1)
#define AUTH_LOCKED (-2)
#define AUTH_MAX_FAILS 5
#define AUTH_LOCK_SECONDS 60

void auth_hash_password(const char *salt, const char *pass, char out[65]);
int auth_check(const char *user, const char *pass, long now);
void auth_reset(void);
int base64_decode(const char *in, char *out, size_t cap);

void audit_log(const char *who, const char *action, const char *result);
int audit_dump_json(char *buf, size_t cap);
#endif
