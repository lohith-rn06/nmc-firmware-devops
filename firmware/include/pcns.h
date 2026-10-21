/* PCNS - PowerChute Network Shutdown style notifications to registered servers. */
#ifndef NMC_PCNS_H
#define NMC_PCNS_H
#include "ups_model.h"
#include <stddef.h>
void pcns_init(void);
void pcns_on_status(ups_status_t old_s, ups_status_t new_s);
int pcns_format_event(char *buf, size_t cap, ups_status_t s);
#endif
