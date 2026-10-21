#include "modbus_tcp.h"
#include "nmc.h"
#include "ups_model.h"
#include <math.h>

static uint16_t u16(double v) { return v <= 0 ? 0 : (v >= 65535 ? 65535 : (uint16_t)lround(v)); }

void modbus_fill_registers(uint16_t r[MB_REG_COUNT])
{
    ups_data_t d;
    ups_get(&d);
    r[MB_STATUS] = (uint16_t)d.status;
    for (int i = 0; i < 3; i++) {
        r[MB_IN_V1 + i] = u16(d.in_v[i] * 10);
        r[MB_OUT_V1 + i] = u16(d.out_v[i] * 10);
        r[MB_OUT_A1 + i] = u16(d.out_a[i] * 10);
    }
    r[MB_IN_FREQ] = u16(d.in_freq * 100);
    r[MB_OUT_FREQ] = u16(d.out_freq * 100);
    r[MB_LOAD] = (uint16_t)d.load_pct;
    r[MB_BATT_PCT] = (uint16_t)d.battery_pct;
    r[MB_RUNTIME] = (uint16_t)d.runtime_min;
    r[MB_BATT_TEMP] = u16(d.battery_temp_c * 10);
    r[MB_UPTIME_HI] = (uint16_t)(d.uptime_s >> 16);
    r[MB_UPTIME_LO] = (uint16_t)(d.uptime_s & 0xffff);
    r[MB_RATED_KVA] = NMC_RATED_KVA;
    r[MB_MODEL_ID] = NMC_MODEL_ID;
}
