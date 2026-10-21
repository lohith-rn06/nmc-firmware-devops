#include "modbus_tcp.h"
#include "test_helpers.h"
#include "unity_lite.h"
#include "ups_model.h"

static size_t req_read(uint8_t *r, uint8_t fc, uint16_t addr, uint16_t qty)
{
    const uint8_t t[] = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, fc, (uint8_t)(addr >> 8), (uint8_t)addr, (uint8_t)(qty >> 8), (uint8_t)qty};
    memcpy(r, t, sizeof t);
    return sizeof t;
}

static void test_read_status_and_model(void)
{
    uint8_t q[12], a[260];
    size_t n = modbus_process(q, req_read(q, 0x03, 0, MB_REG_COUNT), a, sizeof a);
    TEST_ASSERT_EQ(n, 9 + 2 * MB_REG_COUNT);
    TEST_ASSERT_EQ(a[7], 0x03);
    TEST_ASSERT_EQ((a[9] << 8) | a[10], UPS_ONLINE);
    int model = (a[9 + 2 * MB_MODEL_ID] << 8) | a[10 + 2 * MB_MODEL_ID];
    TEST_ASSERT_EQ(model, NMC_MODEL_ID);
}

static void test_out_of_range_is_exception_02(void)
{
    uint8_t q[12], a[260];
    size_t n = modbus_process(q, req_read(q, 0x04, 18, 5), a, sizeof a);
    TEST_ASSERT_EQ(n, 9);
    TEST_ASSERT_EQ(a[7], 0x84);
    TEST_ASSERT_EQ(a[8], 0x02);
}

static void test_write_disabled_by_default(void)
{
    uint8_t q[12], a[260];
    req_read(q, 0x06, 100, 1);
    size_t n = modbus_process(q, 12, a, sizeof a);
    TEST_ASSERT_EQ(n, 9);
    TEST_ASSERT_EQ(a[7], 0x86);
    TEST_ASSERT_EQ(a[8], 0x01);
}

static void test_malformed_length_dropped(void)
{
    uint8_t q[12], a[260];
    req_read(q, 0x03, 0, 1);
    q[5] = 0x20; /* MBAP length lies */
    TEST_ASSERT_EQ(modbus_process(q, 12, a, sizeof a), 0);
}

int main(void)
{
    test_config();
    ups_init();
    RUN_TEST(test_read_status_and_model);
    RUN_TEST(test_out_of_range_is_exception_02);
    RUN_TEST(test_write_disabled_by_default);
    RUN_TEST(test_malformed_length_dropped);
    return TEST_END();
}
