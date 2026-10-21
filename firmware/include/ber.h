/* Minimal ASN.1 BER encoder/decoder used by the SNMP agent. */
#ifndef NMC_BER_H
#define NMC_BER_H
#include <stddef.h>
#include <stdint.h>

#define BER_INTEGER 0x02
#define BER_OCTETSTR 0x04
#define BER_NULL 0x05
#define BER_OID 0x06
#define BER_SEQUENCE 0x30
#define BER_GAUGE32 0x42
#define BER_TIMETICKS 0x43
#define BER_NOSUCHOBJ 0x80
#define BER_ENDOFMIB 0x82

#define OID_MAX 32

int ber_read_tl(const uint8_t *buf, size_t len, size_t *pos, uint8_t *tag, size_t *vlen);
int ber_decode_int(const uint8_t *v, size_t vlen, long *out);
int ber_decode_oid(const uint8_t *v, size_t vlen, uint32_t *oid, size_t *n);
size_t ber_tlv(uint8_t *out, size_t cap, uint8_t tag, const uint8_t *val, size_t vlen);
size_t ber_enc_int(uint8_t *out, size_t cap, uint8_t tag, long v);
size_t ber_enc_uint(uint8_t *out, size_t cap, uint8_t tag, uint32_t v);
size_t ber_enc_oid(uint8_t *out, size_t cap, const uint32_t *oid, size_t n);
int oid_cmp(const uint32_t *a, size_t an, const uint32_t *b, size_t bn);
#endif
