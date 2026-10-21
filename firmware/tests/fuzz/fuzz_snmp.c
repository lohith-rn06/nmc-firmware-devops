/* libFuzzer harness: throws random bytes at the SNMP parser (runs in the nightly pipeline). */
#include "nmc.h"
#include "snmp_agent.h"
#include "ups_model.h"
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static int init;
    if (!init) {
        strcpy(g_cfg.snmp_community, "lab-ro");
        ups_init();
        mib_init();
        init = 1;
    }
    uint8_t out[1500];
    snmp_process(data, size, out, sizeof out);
    return 0;
}
