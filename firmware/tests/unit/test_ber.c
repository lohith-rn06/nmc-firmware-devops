#include "ber.h"
#include "unity_lite.h"

static void test_oid_encode_sysdescr(void)
{
    const uint32_t oid[] = {1, 3, 6, 1, 2, 1, 1, 1, 0};
    const uint8_t want[] = {0x06, 0x08, 0x2b, 0x06, 0x01, 0x02, 0x01, 0x01, 0x01, 0x00};
    uint8_t out[32];
    TEST_ASSERT_EQ(ber_enc_oid(out, sizeof out, oid, 9), sizeof want);
    TEST_ASSERT(memcmp(out, want, sizeof want) == 0);
}

static void test_oid_roundtrip_large_subid(void)
{
    const uint32_t oid[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4294967295u};
    uint8_t out[64];
    uint32_t back[OID_MAX];
    size_t n, pos = 0, vlen;
    uint8_t tag;
    size_t w = ber_enc_oid(out, sizeof out, oid, 11);
    TEST_ASSERT(w > 0);
    TEST_ASSERT_EQ(ber_read_tl(out, w, &pos, &tag, &vlen), 0);
    TEST_ASSERT_EQ(ber_decode_oid(out + pos, vlen, back, &n), 0);
    TEST_ASSERT_EQ(n, 11);
    TEST_ASSERT(back[10] == 4294967295u);
}

static void test_int_encoding(void)
{
    uint8_t out[16];
    long v;
    size_t pos = 0, vlen;
    uint8_t tag;
    TEST_ASSERT_EQ(ber_enc_int(out, sizeof out, BER_INTEGER, 128), 4); /* 02 02 00 80 */
    TEST_ASSERT_EQ(out[2], 0x00);
    TEST_ASSERT_EQ(ber_enc_int(out, sizeof out, BER_INTEGER, -1), 3); /* 02 01 ff */
    TEST_ASSERT_EQ(ber_read_tl(out, 3, &pos, &tag, &vlen), 0);
    TEST_ASSERT_EQ(ber_decode_int(out + pos, vlen, &v), 0);
    TEST_ASSERT_EQ(v, -1);
}

static void test_long_length(void)
{
    uint8_t val[300] = {0}, out[310];
    size_t pos = 0, vlen;
    uint8_t tag;
    size_t w = ber_tlv(out, sizeof out, BER_OCTETSTR, val, sizeof val);
    TEST_ASSERT_EQ(w, 304);
    TEST_ASSERT_EQ(out[1], 0x82);
    TEST_ASSERT_EQ(ber_read_tl(out, w, &pos, &tag, &vlen), 0);
    TEST_ASSERT_EQ(vlen, 300);
}

static void test_rejects_truncated(void)
{
    const uint8_t bad[] = {0x04, 0x10, 0x01}; /* claims 16 bytes, has 1 */
    size_t pos = 0, vlen;
    uint8_t tag;
    TEST_ASSERT_EQ(ber_read_tl(bad, sizeof bad, &pos, &tag, &vlen), -1);
}

int main(void)
{
    RUN_TEST(test_oid_encode_sysdescr);
    RUN_TEST(test_oid_roundtrip_large_subid);
    RUN_TEST(test_int_encoding);
    RUN_TEST(test_long_length);
    RUN_TEST(test_rejects_truncated);
    return TEST_END();
}
