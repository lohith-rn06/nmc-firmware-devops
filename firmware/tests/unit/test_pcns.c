#include "pcns.h"
#include "test_helpers.h"
#include "unity_lite.h"

static void test_on_battery_event_json(void)
{
    char buf[512];
    ups_init();
    int n = pcns_format_event(buf, sizeof buf, UPS_ON_BATTERY);
    TEST_ASSERT(n > 0);
    TEST_ASSERT(strstr(buf, "\"event\":\"ON_BATTERY\"") != NULL);
    TEST_ASSERT(strstr(buf, "\"action\":\"PREPARE_SHUTDOWN\"") != NULL);
}

static void test_restored_event(void)
{
    char buf[512];
    pcns_format_event(buf, sizeof buf, UPS_ONLINE);
    TEST_ASSERT(strstr(buf, "POWER_RESTORED") != NULL);
}

int main(void)
{
    test_config();
    RUN_TEST(test_on_battery_event_json);
    RUN_TEST(test_restored_event);
    return TEST_END();
}
