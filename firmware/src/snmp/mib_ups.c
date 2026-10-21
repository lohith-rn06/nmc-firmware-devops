/* MIB implementation: MIB-II system group + RFC 1628 UPS-MIB (1.3.6.1.2.1.33). */
#include "nmc.h"
#include "snmp_agent.h"
#include "ups_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void g_sysdescr(snmp_value_t *v, int i)
{
    (void)i;
    v->type = BER_OCTETSTR;
    snprintf(v->s, sizeof v->s, "NMC %s 3-phase UPS network management card, fw %s", NMC_MODEL, NMC_VERSION);
}
static void g_uptime(snmp_value_t *v, int i)
{
    (void)i;
    ups_data_t d;
    ups_get(&d);
    v->type = BER_TIMETICKS;
    v->i = (long)d.uptime_s * 100;
}
static void g_sysname(snmp_value_t *v, int i)
{
    (void)i;
    v->type = BER_OCTETSTR;
    snprintf(v->s, sizeof v->s, "nmc-%s", NMC_MODEL);
}
static void g_mfr(snmp_value_t *v, int i)
{
    (void)i;
    v->type = BER_OCTETSTR;
    snprintf(v->s, sizeof v->s, "NMC Practice Lab");
}
static void g_model(snmp_value_t *v, int i)
{
    (void)i;
    v->type = BER_OCTETSTR;
    snprintf(v->s, sizeof v->s, "%s %dkVA", NMC_MODEL, NMC_RATED_KVA);
}
static void g_upsfw(snmp_value_t *v, int i)
{
    (void)i;
    v->type = BER_OCTETSTR;
    snprintf(v->s, sizeof v->s, "UPS-SIM-1.0");
}
static void g_agentfw(snmp_value_t *v, int i)
{
    (void)i;
    v->type = BER_OCTETSTR;
    snprintf(v->s, sizeof v->s, "%s (%s)", NMC_VERSION, NMC_GIT_SHA);
}
static void g_battstat(snmp_value_t *v, int i)
{
    (void)i;
    ups_data_t d;
    ups_get(&d);
    v->type = BER_INTEGER;
    v->i = d.battery_pct < 20 ? 3 : 2;
}
static void g_runtime(snmp_value_t *v, int i)
{
    (void)i;
    ups_data_t d;
    ups_get(&d);
    v->type = BER_INTEGER;
    v->i = d.runtime_min;
}
static void g_charge(snmp_value_t *v, int i)
{
    (void)i;
    ups_data_t d;
    ups_get(&d);
    v->type = BER_INTEGER;
    v->i = d.battery_pct;
}
static void g_btemp(snmp_value_t *v, int i)
{
    (void)i;
    ups_data_t d;
    ups_get(&d);
    v->type = BER_INTEGER;
    v->i = (long)(d.battery_temp_c + 0.5);
}
static void g_numlines(snmp_value_t *v, int i)
{
    (void)i;
    v->type = BER_INTEGER;
    v->i = 3;
}
static void g_infreq(snmp_value_t *v, int i)
{
    (void)i;
    ups_data_t d;
    ups_get(&d);
    v->type = BER_INTEGER;
    v->i = (long)(d.in_freq * 10 + 0.5);
}
static void g_inv(snmp_value_t *v, int i)
{
    ups_data_t d;
    ups_get(&d);
    v->type = BER_INTEGER;
    v->i = (long)(d.in_v[i] + 0.5);
}
static void g_outsrc(snmp_value_t *v, int i)
{
    (void)i;
    ups_data_t d;
    ups_get(&d);
    v->type = BER_INTEGER;
    v->i = d.status == UPS_ON_BATTERY ? 5 : d.status == UPS_BYPASS ? 4 : 3;
}
static void g_outfreq(snmp_value_t *v, int i)
{
    (void)i;
    ups_data_t d;
    ups_get(&d);
    v->type = BER_INTEGER;
    v->i = (long)(d.out_freq * 10 + 0.5);
}
static void g_outv(snmp_value_t *v, int i)
{
    ups_data_t d;
    ups_get(&d);
    v->type = BER_INTEGER;
    v->i = (long)(d.out_v[i] + 0.5);
}
static void g_outa(snmp_value_t *v, int i)
{
    ups_data_t d;
    ups_get(&d);
    v->type = BER_INTEGER;
    v->i = (long)(d.out_a[i] * 10 + 0.5);
}
static void g_load(snmp_value_t *v, int i)
{
    (void)i;
    ups_data_t d;
    ups_get(&d);
    v->type = BER_INTEGER;
    v->i = d.load_pct;
}

#define O(...)                                                                                                                   \
    {                                                                                                                            \
        __VA_ARGS__                                                                                                              \
    }
static mib_entry_t mib[64];
static size_t nmib;

static void add(const uint32_t *oid, size_t n, void (*get)(snmp_value_t *, int), int idx)
{
    if (nmib >= sizeof mib / sizeof mib[0]) return;
    memcpy(mib[nmib].oid, oid, n * sizeof(uint32_t));
    mib[nmib].n = n;
    mib[nmib].get = get;
    mib[nmib].idx = idx;
    nmib++;
}
#define ADD(get, idx, ...)                                                                                                       \
    do {                                                                                                                         \
        const uint32_t o[] = {__VA_ARGS__};                                                                                      \
        add(o, sizeof o / sizeof o[0], get, idx);                                                                                \
    } while (0)

static int cmp_entry(const void *a, const void *b)
{
    const mib_entry_t *x = a, *y = b;
    return oid_cmp(x->oid, x->n, y->oid, y->n);
}

void mib_init(void)
{
    nmib = 0;
    ADD(g_sysdescr, 0, 1, 3, 6, 1, 2, 1, 1, 1, 0);
    ADD(g_uptime, 0, 1, 3, 6, 1, 2, 1, 1, 3, 0);
    ADD(g_sysname, 0, 1, 3, 6, 1, 2, 1, 1, 5, 0);
    ADD(g_mfr, 0, 1, 3, 6, 1, 2, 1, 33, 1, 1, 1, 0);
    ADD(g_model, 0, 1, 3, 6, 1, 2, 1, 33, 1, 1, 2, 0);
    ADD(g_upsfw, 0, 1, 3, 6, 1, 2, 1, 33, 1, 1, 3, 0);
    ADD(g_agentfw, 0, 1, 3, 6, 1, 2, 1, 33, 1, 1, 4, 0);
    ADD(g_battstat, 0, 1, 3, 6, 1, 2, 1, 33, 1, 2, 1, 0);
    ADD(g_runtime, 0, 1, 3, 6, 1, 2, 1, 33, 1, 2, 3, 0);
    ADD(g_charge, 0, 1, 3, 6, 1, 2, 1, 33, 1, 2, 4, 0);
    ADD(g_btemp, 0, 1, 3, 6, 1, 2, 1, 33, 1, 2, 7, 0);
    ADD(g_numlines, 0, 1, 3, 6, 1, 2, 1, 33, 1, 3, 2, 0);
    ADD(g_outsrc, 0, 1, 3, 6, 1, 2, 1, 33, 1, 4, 1, 0);
    ADD(g_outfreq, 0, 1, 3, 6, 1, 2, 1, 33, 1, 4, 2, 0);
    ADD(g_numlines, 0, 1, 3, 6, 1, 2, 1, 33, 1, 4, 3, 0);
    for (uint32_t p = 1; p <= 3; p++) {
        ADD(g_infreq, (int)p - 1, 1, 3, 6, 1, 2, 1, 33, 1, 3, 3, 1, 2, p);
        ADD(g_inv, (int)p - 1, 1, 3, 6, 1, 2, 1, 33, 1, 3, 3, 1, 3, p);
        ADD(g_outv, (int)p - 1, 1, 3, 6, 1, 2, 1, 33, 1, 4, 4, 1, 2, p);
        ADD(g_outa, (int)p - 1, 1, 3, 6, 1, 2, 1, 33, 1, 4, 4, 1, 3, p);
        ADD(g_load, (int)p - 1, 1, 3, 6, 1, 2, 1, 33, 1, 4, 4, 1, 5, p);
    }
    qsort(mib, nmib, sizeof mib[0], cmp_entry);
}

const mib_entry_t *mib_find(const uint32_t *oid, size_t n)
{
    for (size_t i = 0; i < nmib; i++)
        if (oid_cmp(mib[i].oid, mib[i].n, oid, n) == 0) return &mib[i];
    return NULL;
}

const mib_entry_t *mib_next(const uint32_t *oid, size_t n)
{
    for (size_t i = 0; i < nmib; i++)
        if (oid_cmp(mib[i].oid, mib[i].n, oid, n) > 0) return &mib[i];
    return NULL;
}
