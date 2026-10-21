#include "ber.h"
#include <string.h>

int ber_read_tl(const uint8_t *buf, size_t len, size_t *pos, uint8_t *tag, size_t *vlen)
{
    size_t p = *pos;
    if (p + 2 > len) return -1;
    *tag = buf[p++];
    size_t l = buf[p++];
    if (l & 0x80) {
        size_t nb = l & 0x7f;
        if (nb == 0 || nb > 2 || p + nb > len) return -1; /* we never need > 64 KiB */
        l = 0;
        while (nb--)
            l = (l << 8) | buf[p++];
    }
    if (p + l > len) return -1;
    *vlen = l;
    *pos = p;
    return 0;
}

int ber_decode_int(const uint8_t *v, size_t vlen, long *out)
{
    if (vlen == 0 || vlen > sizeof(long)) return -1;
    long r = (v[0] & 0x80) ? -1 : 0;
    for (size_t i = 0; i < vlen; i++)
        r = (long)(((unsigned long)r << 8) | v[i]);
    *out = r;
    return 0;
}

int ber_decode_oid(const uint8_t *v, size_t vlen, uint32_t *oid, size_t *n)
{
    if (vlen == 0) return -1;
    size_t k = 0;
    oid[k++] = v[0] / 40;
    oid[k++] = v[0] % 40;
    uint32_t acc = 0;
    for (size_t i = 1; i < vlen; i++) {
        if (acc > (UINT32_MAX >> 7)) return -1;
        acc = (acc << 7) | (v[i] & 0x7f);
        if (!(v[i] & 0x80)) {
            if (k >= OID_MAX) return -1;
            oid[k++] = acc;
            acc = 0;
        }
    }
    if (v[vlen - 1] & 0x80) return -1; /* truncated sub-identifier */
    *n = k;
    return 0;
}

size_t ber_tlv(uint8_t *out, size_t cap, uint8_t tag, const uint8_t *val, size_t vlen)
{
    size_t hl = vlen < 0x80 ? 2 : (vlen < 0x100 ? 3 : 4);
    if (vlen > 0xffff || hl + vlen > cap) return 0;
    size_t p = 0;
    out[p++] = tag;
    if (vlen < 0x80)
        out[p++] = (uint8_t)vlen;
    else if (vlen < 0x100) {
        out[p++] = 0x81;
        out[p++] = (uint8_t)vlen;
    } else {
        out[p++] = 0x82;
        out[p++] = (uint8_t)(vlen >> 8);
        out[p++] = (uint8_t)vlen;
    }
    if (vlen) memmove(out + p, val, vlen);
    return p + vlen;
}

size_t ber_enc_int(uint8_t *out, size_t cap, uint8_t tag, long v)
{
    uint8_t b[sizeof(long)];
    size_t n = sizeof(long);
    for (size_t i = 0; i < sizeof(long); i++)
        b[sizeof(long) - 1 - i] = (uint8_t)((unsigned long)v >> (8 * i));
    size_t s = 0; /* strip redundant leading bytes, keep sign */
    while (n - s > 1 && ((b[s] == 0x00 && !(b[s + 1] & 0x80)) || (b[s] == 0xff && (b[s + 1] & 0x80))))
        s++;
    return ber_tlv(out, cap, tag, b + s, n - s);
}

size_t ber_enc_uint(uint8_t *out, size_t cap, uint8_t tag, uint32_t v)
{
    uint8_t b[5] = {0, (uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v};
    size_t s = 0;
    while (s < 4 && b[s] == 0 && !(b[s + 1] & 0x80))
        s++;
    return ber_tlv(out, cap, tag, b + s, 5 - s);
}

size_t ber_enc_oid(uint8_t *out, size_t cap, const uint32_t *oid, size_t n)
{
    uint8_t b[OID_MAX * 5];
    size_t p = 0;
    if (n < 2) return 0;
    b[p++] = (uint8_t)(oid[0] * 40 + oid[1]);
    for (size_t i = 2; i < n; i++) {
        uint8_t tmp[5];
        int t = 0;
        uint32_t x = oid[i];
        do {
            tmp[t++] = x & 0x7f;
            x >>= 7;
        } while (x);
        while (t--)
            b[p++] = (uint8_t)(tmp[t] | (t ? 0x80 : 0));
    }
    return ber_tlv(out, cap, BER_OID, b, p);
}

int oid_cmp(const uint32_t *a, size_t an, const uint32_t *b, size_t bn)
{
    size_t n = an < bn ? an : bn;
    for (size_t i = 0; i < n; i++)
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    return (an > bn) - (an < bn);
}
