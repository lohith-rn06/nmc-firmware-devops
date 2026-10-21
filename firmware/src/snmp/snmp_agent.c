/* SNMP v1/v2c agent: GET and GETNEXT (enough for snmpget/snmpwalk). SNMPv3 (USM) is
 * listed as roadmap work - see docs/adr/0004-snmpv3.md. */
#include "snmp_agent.h"
#include "log.h"
#include "nmc.h"
#include "security.h"
#include "tcpip.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PDU_GET 0xA0
#define PDU_GETNEXT 0xA1
#define PDU_RESPONSE 0xA2
#define MAX_VB 16
/* encode one element, bail out on overflow */
#define APPEND(off, expr)                                                                                                        \
    do {                                                                                                                         \
        size_t _w = (expr);                                                                                                      \
        if (!_w) return 0;                                                                                                       \
        (off) += _w;                                                                                                             \
    } while (0)

static size_t enc_value(uint8_t *out, size_t cap, const snmp_value_t *v)
{
    switch (v->type) {
    case BER_OCTETSTR:
        return ber_tlv(out, cap, BER_OCTETSTR, (const uint8_t *)v->s, strlen(v->s));
    case BER_TIMETICKS:
    case BER_GAUGE32:
        return ber_enc_uint(out, cap, v->type, (uint32_t)v->i);
    case BER_INTEGER:
        return ber_enc_int(out, cap, BER_INTEGER, v->i);
    default:
        return ber_tlv(out, cap, v->type, NULL, 0); /* NULL / exceptions */
    }
}

size_t snmp_process(const uint8_t *req, size_t len, uint8_t *resp, size_t cap)
{
    size_t pos = 0, vlen, end;
    uint8_t tag;
    long version, reqid, tmp;
    char community[64];

    if (ber_read_tl(req, len, &pos, &tag, &vlen) || tag != BER_SEQUENCE) return 0;
    if (ber_read_tl(req, len, &pos, &tag, &vlen) || tag != BER_INTEGER || ber_decode_int(req + pos, vlen, &version)) return 0;
    pos += vlen;
    if (version != 0 && version != 1) return 0; /* v1 or v2c only */
    if (ber_read_tl(req, len, &pos, &tag, &vlen) || tag != BER_OCTETSTR || vlen >= sizeof community) return 0;
    memcpy(community, req + pos, vlen);
    community[vlen] = 0;
    pos += vlen;
    if (strcmp(community, g_cfg.snmp_community) != 0) { /* RFC: silently drop */
        audit_log("snmp", "bad-community", "denied");
        return 0;
    }
    uint8_t pdu;
    if (ber_read_tl(req, len, &pos, &pdu, &vlen) || (pdu != PDU_GET && pdu != PDU_GETNEXT)) return 0;
    if (ber_read_tl(req, len, &pos, &tag, &vlen) || tag != BER_INTEGER || ber_decode_int(req + pos, vlen, &reqid)) return 0;
    pos += vlen;
    for (int k = 0; k < 2; k++) { /* error-status, error-index */
        if (ber_read_tl(req, len, &pos, &tag, &vlen) || tag != BER_INTEGER || ber_decode_int(req + pos, vlen, &tmp)) return 0;
        pos += vlen;
    }
    if (ber_read_tl(req, len, &pos, &tag, &vlen) || tag != BER_SEQUENCE) return 0;
    end = pos + vlen;

    uint8_t vbs[1200];
    size_t vbn = 0;
    long err_status = 0, err_index = 0;
    int count = 0;
    while (pos < end) {
        uint32_t oid[OID_MAX];
        size_t on;
        size_t vbend;
        if (++count > MAX_VB) return 0;
        if (ber_read_tl(req, end, &pos, &tag, &vlen) || tag != BER_SEQUENCE) return 0;
        vbend = pos + vlen;
        if (ber_read_tl(req, vbend, &pos, &tag, &vlen) || tag != BER_OID || ber_decode_oid(req + pos, vlen, oid, &on)) return 0;
        pos = vbend; /* skip the (NULL) value */

        const mib_entry_t *e = (pdu == PDU_GET) ? mib_find(oid, on) : mib_next(oid, on);
        snmp_value_t val;
        memset(&val, 0, sizeof val);
        const uint32_t *roid = oid;
        size_t roid_n = on;
        if (e) {
            e->get(&val, e->idx);
            roid = e->oid;
            roid_n = e->n;
        } else if (version == 1)
            val.type = (pdu == PDU_GET) ? BER_NOSUCHOBJ : BER_ENDOFMIB;
        else {
            val.type = BER_NULL;
            if (!err_status) {
                err_status = 2;
                err_index = count;
            }
        } /* v1 noSuchName */

        uint8_t one[256];
        size_t n1 = ber_enc_oid(one, sizeof one, roid, roid_n);
        size_t n2 = n1 ? enc_value(one + n1, sizeof one - n1, &val) : 0;
        if (!n2) return 0;
        size_t w = ber_tlv(vbs + vbn, sizeof vbs - vbn, BER_SEQUENCE, one, n1 + n2);
        if (!w) return 0;
        vbn += w;
    }

    uint8_t body[1400];
    size_t b = 0;
    APPEND(b, ber_enc_int(body + b, sizeof body - b, BER_INTEGER, reqid));
    APPEND(b, ber_enc_int(body + b, sizeof body - b, BER_INTEGER, err_status));
    APPEND(b, ber_enc_int(body + b, sizeof body - b, BER_INTEGER, err_index));
    APPEND(b, ber_tlv(body + b, sizeof body - b, BER_SEQUENCE, vbs, vbn));

    uint8_t msg[1480];
    size_t mlen = 0;
    APPEND(mlen, ber_enc_int(msg + mlen, sizeof msg - mlen, BER_INTEGER, version));
    APPEND(mlen, ber_tlv(msg + mlen, sizeof msg - mlen, BER_OCTETSTR, (const uint8_t *)community, strlen(community)));
    APPEND(mlen, ber_tlv(msg + mlen, sizeof msg - mlen, PDU_RESPONSE, body, b));
    return ber_tlv(resp, cap, BER_SEQUENCE, msg, mlen);
}

static void *snmp_thread(void *arg)
{
    int fd = (int)(intptr_t)arg;
    uint8_t in[1500], out[1500];
    for (;;) {
        struct sockaddr_storage peer;
        socklen_t pl = sizeof peer;
        ssize_t n = recvfrom(fd, in, sizeof in, 0, (struct sockaddr *)&peer, &pl);
        if (n <= 0) continue;
        size_t r = snmp_process(in, (size_t)n, out, sizeof out);
        if (r) sendto(fd, out, r, 0, (struct sockaddr *)&peer, pl);
    }
    return NULL;
}

int snmp_start(void)
{
    mib_init();
    int fd = udp_bind(g_cfg.snmp_port);
    if (fd < 0) return -1;
    pthread_t t;
    if (pthread_create(&t, NULL, snmp_thread, (void *)(intptr_t)fd) != 0) return -1;
    pthread_detach(t);
    nmc_log(LOG_INF, "snmp", "agent listening on udp/%d (v1/v2c)", g_cfg.snmp_port);
    return 0;
}
