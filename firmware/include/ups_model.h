/* Simulated 3-phase UPS data model (what the NMC reads from the UPS over its internal bus). */
#ifndef NMC_UPS_MODEL_H
#define NMC_UPS_MODEL_H
#include <stdint.h>

typedef enum { UPS_ONLINE = 2, UPS_ON_BATTERY = 3, UPS_BYPASS = 4 } ups_status_t;

typedef struct {
    ups_status_t status;
    double in_v[3];  /* phase-neutral input voltage, V */
    double out_v[3]; /* phase-neutral output voltage, V */
    double out_a[3]; /* output current per phase, A */
    double in_freq;  /* Hz */
    double out_freq; /* Hz */
    int load_pct;
    int battery_pct;
    int runtime_min;
    double battery_temp_c;
    uint32_t uptime_s;
} ups_data_t;

typedef void (*ups_status_cb)(ups_status_t old_s, ups_status_t new_s);

void ups_init(void);
void ups_tick(void); /* advance simulation by one second */
void ups_get(ups_data_t *out);
void ups_set_status(ups_status_t s);
void ups_on_status_change(ups_status_cb cb);
const char *ups_status_str(ups_status_t s);
#endif
