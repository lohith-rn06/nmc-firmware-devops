/* NMC firmware entry point. */
#include "fw_update.h"
#include "log.h"
#include "nmc.h"
#include "ups_model.h"
#ifdef NMC_FEATURE_SNMP
#include "snmp_agent.h"
#endif
#ifdef NMC_FEATURE_MODBUS
#include "modbus_tcp.h"
#endif
#ifdef NMC_FEATURE_HTTP
#include "web_server.h"
#endif
#ifdef NMC_FEATURE_PCNS
#include "pcns.h"
#endif
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t running = 1;
static void on_signal(int s)
{
    (void)s;
    running = 0;
}

int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp(argv[1], "--version")) {
        printf("%s %s (%s) features=%s\n", NMC_MODEL, NMC_VERSION, NMC_GIT_SHA, nmc_features());
        return 0;
    }
    if (argc > 2 && !strcmp(argv[1], "--validate")) {
        char err[160];
        int rc = fw_validate_file(argv[2], err, sizeof err);
        printf("%s: %s\n", argv[2], err);
        return rc == 0 ? 0 : 2;
    }

    signal(SIGTERM, on_signal);
    signal(SIGINT, on_signal);
    signal(SIGPIPE, SIG_IGN);
    config_load(&g_cfg);
    ups_init();
    nmc_log(LOG_INF, "main", "NMC %s firmware %s starting (features: %s)", NMC_MODEL, NMC_VERSION, nmc_features());

    int failed = 0;
#ifdef NMC_FEATURE_PCNS
    pcns_init();
#endif
#ifdef NMC_FEATURE_SNMP
    failed |= snmp_start() != 0;
#endif
#ifdef NMC_FEATURE_MODBUS
    failed |= modbus_start() != 0;
#endif
#ifdef NMC_FEATURE_HTTP
    failed |= http_start() != 0;
#endif
    if (failed) {
        nmc_log(LOG_ERR, "main", "a service failed to start");
        return 1;
    }

    while (running) {
        sleep(1);
        ups_tick();
    }
    nmc_log(LOG_INF, "main", "shutting down");
    return 0;
}
