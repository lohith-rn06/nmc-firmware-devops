/* Runtime configuration. On real hardware this comes from the card's flash config;
 * in the simulator it is read from environment variables (12-factor style). */
#include "log.h"
#include "nmc.h"
#include "security.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

nmc_config_t g_cfg;

static int env_int(const char *k, int def)
{
    const char *v = getenv(k);
    if (!v || !*v) return def;
    char *end;
    long n = strtol(v, &end, 10);
    if (*end || n < 0 || n > 65535) {
        nmc_log(LOG_WRN, "config", "ignoring invalid %s", k);
        return def;
    }
    return (int)n;
}
static void env_str(char *dst, size_t cap, const char *k, const char *def)
{
    const char *v = getenv(k);
    snprintf(dst, cap, "%s", v ? v : def);
}

/* Fail secure: if the kernel RNG is unavailable we refuse to start rather than
 * fall back to a predictable generator. */
static void random_hex(char *out, size_t nbytes)
{
    unsigned char b[32];
    if (nbytes > sizeof b) nbytes = sizeof b;
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f || fread(b, 1, nbytes, f) != nbytes) {
        nmc_log(LOG_ERR, "config", "no entropy source available");
        exit(EXIT_FAILURE);
    }
    fclose(f);
    for (size_t i = 0; i < nbytes; i++)
        snprintf(out + 2 * i, 3, "%02x", b[i]);
}

void config_load(nmc_config_t *c)
{
    memset(c, 0, sizeof *c);
    c->snmp_port = env_int("NMC_SNMP_PORT", 1161);
    c->modbus_port = env_int("NMC_MODBUS_PORT", 1502);
    c->http_port = env_int("NMC_HTTP_PORT", 8080);
    c->modbus_write_enabled = env_int("NMC_MODBUS_WRITE", 0);
    c->simulation = env_int("NMC_SIMULATION", 0);
    env_str(c->snmp_community, sizeof c->snmp_community, "NMC_SNMP_COMMUNITY", "");
    env_str(c->admin_user, sizeof c->admin_user, "NMC_ADMIN_USER", "admin");
    env_str(c->pcns_clients, sizeof c->pcns_clients, "NMC_PCNS_CLIENTS", "");
    random_hex(c->salt, 8);

    /* Secure-by-default: no factory default password (IEC 62443-4-2 CR 1.5).
     * If none is provisioned, a one-time random password is generated. */
    char pass[65];
    const char *p = getenv("NMC_ADMIN_PASS");
    if (p && *p)
        snprintf(pass, sizeof pass, "%s", p);
    else {
        random_hex(pass, 8);
        nmc_log(LOG_WRN, "config", "no admin password provisioned; one-time password: %s", pass);
    }
    auth_hash_password(c->salt, pass, c->admin_pass_hash);
    memset(pass, 0, sizeof pass);

    if (!c->snmp_community[0]) {
        random_hex(c->snmp_community, 8);
        nmc_log(LOG_WRN, "config", "no SNMP community provisioned; generated random community");
    }
}

const char *nmc_features(void)
{
    return ""
#ifdef NMC_FEATURE_SNMP
           "snmp,"
#endif
#ifdef NMC_FEATURE_MODBUS
           "modbus,"
#endif
#ifdef NMC_FEATURE_HTTP
           "http,"
#endif
#ifdef NMC_FEATURE_PCNS
           "pcns,"
#endif
#ifdef NMC_FEATURE_PARALLEL
           "parallel-ups,"
#endif
#ifdef NMC_FEATURE_FIPS
           "fips-mode,"
#endif
           "security,ota";
}
