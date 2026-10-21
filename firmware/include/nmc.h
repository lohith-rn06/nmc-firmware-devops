/* nmc.h - common definitions for the NMC (Network Management Card) firmware.
 * Build-time identity is injected by CMake from variants/<card>.cmake. */
#ifndef NMC_H
#define NMC_H
#include <stddef.h>
#include <stdint.h>

#ifndef NMC_MODEL
#define NMC_MODEL "GVS"
#endif
#ifndef NMC_MODEL_ID
#define NMC_MODEL_ID 1
#endif
#ifndef NMC_VERSION
#define NMC_VERSION "0.0.0-dev"
#endif
#ifndef NMC_GIT_SHA
#define NMC_GIT_SHA "unknown"
#endif
#ifndef NMC_NOMINAL_VOLTAGE /* phase-to-phase, volts */
#define NMC_NOMINAL_VOLTAGE 400
#endif
#ifndef NMC_NOMINAL_FREQ /* Hz */
#define NMC_NOMINAL_FREQ 50
#endif
#ifndef NMC_RATED_KVA
#define NMC_RATED_KVA 100
#endif

typedef struct {
    int snmp_port;
    int modbus_port;
    int http_port;
    int modbus_write_enabled; /* writes are OFF by default (secure default) */
    int simulation;           /* enables the simulate API - lab builds only */
    char snmp_community[32];
    char admin_user[32];
    char salt[17];
    char admin_pass_hash[65]; /* hex(sha256(salt || password)) */
    char pcns_clients[256];   /* comma separated host:port list */
} nmc_config_t;

extern nmc_config_t g_cfg;

void config_load(nmc_config_t *c);
const char *nmc_features(void); /* comma separated list of compiled features */

#endif
