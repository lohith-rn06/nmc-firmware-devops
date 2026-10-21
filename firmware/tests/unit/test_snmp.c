#include "snmp_agent.h"
#include "test_helpers.h"
#include "unity_lite.h"
#include "ups_model.h"

/* Build an SNMP GET/GETNEXT request with one varbind. */
static size_t build_req(uint8_t *out, long version, const char *comm, uint8_t pdu, const uint32_t *oid, size_t on)
{
    uint8_t vb[64], vbs[80], body[128], msg[200], tmp[64];
    size_t n, b = 0, m = 0;
    n = ber_enc_oid(tmp, sizeof tmp, oid, on);
    n += ber_tlv(tmp + n, sizeof tmp - n, BER_NULL, NULL, 0);
    size_t v1 = ber_tlv(vb, sizeof vb, BER_SEQUENCE, tmp, n);
    size_t v2 = ber_tlv(vbs, sizeof vbs, BER_SEQUENCE, vb, v1);
    b += ber_enc_int(body + b, sizeof body - b, BER_INTEGER, 4242);
    b += ber_enc_int(body + b, sizeof body - b, BER_INTEGER, 0);
    b += ber_enc_int(body + b, sizeof body - b, BER_INTEGER, 0);
    memcpy(body + b, vbs, v2);
    b += v2;
    m += ber_enc_int(msg + m, sizeof msg - m, BER_INTEGER, version);
    m += ber_tlv(msg + m, sizeof msg - m, BER_OCTETSTR, (const uint8_t *)comm, strlen(comm));
    m += ber_tlv(msg + m, sizeof msg - m, pdu, body, b);
    return ber_tlv(out, 256, BER_SEQUENCE, msg, m);
}

static int contains(const uint8_t *h, size_t hn, const char *needle)
{
    size_t nn = strlen(needle);
    for (size_t i = 0; i + nn <= hn; i++)
        if (!memcmp(h + i, needle, nn)) return 1;
    return 0;
}

static void test_get_sysdescr(void)
{
    const uint32_t oid[] = {1, 3, 6, 1, 2, 1, 1, 1, 0};
    uint8_t req[256], resp[1500];
    size_t rl = build_req(req, 1, "lab-ro", 0xA0, oid, 9);
    size_t n = snmp_process(req, rl, resp, sizeof resp);
    TEST_ASSERT(n > 0);
    TEST_ASSERT(contains(resp, n, "NMC " NMC_MODEL));
}

static void test_wrong_community_dropped(void)
{
    const uint32_t oid[] = {1, 3, 6, 1, 2, 1, 1, 1, 0};
    uint8_t req[256], resp[1500];
    size_t rl = build_req(req, 1, "public", 0xA0, oid, 9);
    TEST_ASSERT_EQ(snmp_process(req, rl, resp, sizeof resp), 0);
}

static void test_getnext_walks_into_ups_mib(void)
{
    const uint32_t oid[] = {1, 3, 6, 1, 2, 1, 33};
    uint8_t req[256], resp[1500];
    size_t rl = build_req(req, 1, "lab-ro", 0xA1, oid, 7);
    size_t n = snmp_process(req, rl, resp, sizeof resp);
    TEST_ASSERT(n > 0);
    TEST_ASSERT(contains(resp, n, "NMC Practice Lab")); /* upsIdentManufacturer */
}

static void test_garbage_is_ignored(void)
{
    uint8_t junk[] = {0x30, 0x82, 0xff, 0xff, 0x02}, resp[64];
    TEST_ASSERT_EQ(snmp_process(junk, sizeof junk, resp, sizeof resp), 0);
}

int main(void)
{
    test_config();
    ups_init();
    mib_init();
    RUN_TEST(test_get_sysdescr);
    RUN_TEST(test_wrong_community_dropped);
    RUN_TEST(test_getnext_walks_into_ups_mib);
    RUN_TEST(test_garbage_is_ignored);
    return TEST_END();
}
