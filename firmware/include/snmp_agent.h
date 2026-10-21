#ifndef NMC_SNMP_AGENT_H
#define NMC_SNMP_AGENT_H
#include "ber.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t type;
    long i;
    char s[96];
} snmp_value_t;
typedef struct {
    uint32_t oid[OID_MAX];
    size_t n;
    void (*get)(snmp_value_t *v, int idx);
    int idx;
} mib_entry_t;

void mib_init(void);
const mib_entry_t *mib_find(const uint32_t *oid, size_t n);
const mib_entry_t *mib_next(const uint32_t *oid, size_t n);
size_t snmp_process(const uint8_t *req, size_t len, uint8_t *resp, size_t cap);
int snmp_start(void);
#endif
