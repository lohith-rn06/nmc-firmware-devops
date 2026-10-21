#ifndef NMC_MODBUS_TCP_H
#define NMC_MODBUS_TCP_H
#include <stddef.h>
#include <stdint.h>

#define MB_REG_COUNT 20
/* Holding/input register map - see firmware/mibs/modbus-register-map.csv */
enum {
    MB_STATUS = 0,
    MB_IN_V1,
    MB_IN_V2,
    MB_IN_V3,
    MB_OUT_V1,
    MB_OUT_V2,
    MB_OUT_V3,
    MB_OUT_A1,
    MB_OUT_A2,
    MB_OUT_A3,
    MB_IN_FREQ,
    MB_OUT_FREQ,
    MB_LOAD,
    MB_BATT_PCT,
    MB_RUNTIME,
    MB_BATT_TEMP,
    MB_UPTIME_HI,
    MB_UPTIME_LO,
    MB_RATED_KVA,
    MB_MODEL_ID
};
void modbus_fill_registers(uint16_t regs[MB_REG_COUNT]);
size_t modbus_process(const uint8_t *req, size_t len, uint8_t *resp, size_t cap);
int modbus_start(void);
#endif
