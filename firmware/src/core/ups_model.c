#include "ups_model.h"
#include "nmc.h"
#include <math.h>
#include <pthread.h>
#include <string.h>

#define MAX_CBS 4
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static ups_data_t d;
static ups_status_cb cbs[MAX_CBS];
static int ncbs;
static unsigned seed = 12345;

static double jitter(double span) /* deterministic pseudo-random in [-span, span] */
{
    seed = seed * 1103515245u + 12345u;
    return ((double)((seed >> 16) & 0x7fff) / 32767.0 * 2.0 - 1.0) * span;
}

void ups_init(void)
{
    pthread_mutex_lock(&lock);
    memset(&d, 0, sizeof d);
    double vpn = NMC_NOMINAL_VOLTAGE / sqrt(3.0);
    double amps = (NMC_RATED_KVA * 1000.0 / 3.0) / vpn * 0.45; /* ~45% load */
    d.status = UPS_ONLINE;
    for (int i = 0; i < 3; i++) {
        d.in_v[i] = vpn;
        d.out_v[i] = vpn;
        d.out_a[i] = amps;
    }
    d.in_freq = d.out_freq = NMC_NOMINAL_FREQ;
    d.load_pct = 45;
    d.battery_pct = 100;
    d.runtime_min = 30;
    d.battery_temp_c = 24.0;
    ncbs = 0;
    pthread_mutex_unlock(&lock);
}

void ups_tick(void)
{
    pthread_mutex_lock(&lock);
    double vpn = NMC_NOMINAL_VOLTAGE / sqrt(3.0);
    d.uptime_s++;
    for (int i = 0; i < 3; i++) {
        d.in_v[i] = (d.status == UPS_ON_BATTERY) ? 0.0 : vpn + jitter(2.0);
        d.out_v[i] = vpn + jitter(0.5);
        d.out_a[i] = (NMC_RATED_KVA * 1000.0 / 3.0) / vpn * (d.load_pct / 100.0) + jitter(1.0);
    }
    d.in_freq = (d.status == UPS_ON_BATTERY) ? 0.0 : NMC_NOMINAL_FREQ + jitter(0.05);
    d.out_freq = NMC_NOMINAL_FREQ + jitter(0.01);
    d.battery_temp_c = 24.0 + jitter(0.3);
    if (d.status == UPS_ON_BATTERY) {
        if (d.uptime_s % 10 == 0 && d.battery_pct > 0) d.battery_pct--;
    } else if (d.battery_pct < 100 && d.uptime_s % 5 == 0) {
        d.battery_pct++;
    }
    d.runtime_min = d.battery_pct * 30 / 100;
    pthread_mutex_unlock(&lock);
}

void ups_get(ups_data_t *out)
{
    pthread_mutex_lock(&lock);
    *out = d;
    pthread_mutex_unlock(&lock);
}

void ups_on_status_change(ups_status_cb cb)
{
    pthread_mutex_lock(&lock);
    if (ncbs < MAX_CBS) cbs[ncbs++] = cb;
    pthread_mutex_unlock(&lock);
}

void ups_set_status(ups_status_t s)
{
    pthread_mutex_lock(&lock);
    ups_status_t old = d.status;
    d.status = s;
    int n = ncbs;
    ups_status_cb local[MAX_CBS];
    for (int i = 0; i < n; i++)
        local[i] = cbs[i];
    pthread_mutex_unlock(&lock);
    if (old != s)
        for (int i = 0; i < n; i++)
            local[i](old, s); /* call outside the lock */
}

const char *ups_status_str(ups_status_t s)
{
    switch (s) {
    case UPS_ONLINE:
        return "ONLINE";
    case UPS_ON_BATTERY:
        return "ON_BATTERY";
    case UPS_BYPASS:
        return "BYPASS";
    }
    return "UNKNOWN";
}
