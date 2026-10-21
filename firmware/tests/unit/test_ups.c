#include "test_helpers.h"
#include "unity_lite.h"
#include "ups_model.h"
#include <math.h>

static int calls;
static ups_status_t last;
static void cb(ups_status_t o, ups_status_t n)
{
    (void)o;
    calls++;
    last = n;
}

static void test_nominal_values(void)
{
    ups_data_t d;
    ups_init();
    ups_get(&d);
    TEST_ASSERT_EQ(d.status, UPS_ONLINE);
    TEST_ASSERT(fabs(d.out_v[0] - NMC_NOMINAL_VOLTAGE / sqrt(3.0)) < 1.0);
    TEST_ASSERT(fabs(d.out_freq - NMC_NOMINAL_FREQ) < 0.1);
    TEST_ASSERT_EQ(d.battery_pct, 100);
}

static void test_status_callback_fires_once_per_change(void)
{
    ups_init();
    calls = 0;
    ups_on_status_change(cb);
    ups_set_status(UPS_ON_BATTERY);
    ups_set_status(UPS_ON_BATTERY);
    TEST_ASSERT_EQ(calls, 1);
    TEST_ASSERT_EQ(last, UPS_ON_BATTERY);
}

static void test_battery_drains_on_battery(void)
{
    ups_data_t d;
    ups_init();
    ups_set_status(UPS_ON_BATTERY);
    for (int i = 0; i < 60; i++)
        ups_tick();
    ups_get(&d);
    TEST_ASSERT(d.battery_pct < 100);
    TEST_ASSERT(d.in_v[0] == 0.0);
}

int main(void)
{
    test_config();
    RUN_TEST(test_nominal_values);
    RUN_TEST(test_status_callback_fires_once_per_change);
    RUN_TEST(test_battery_drains_on_battery);
    return TEST_END();
}
