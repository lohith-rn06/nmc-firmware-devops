#include "test_helpers.h"
#include "unity_lite.h"

static void test_sha256_vectors(void)
{
    char h[65];
    sha256_hex((const uint8_t *)"abc", 3, h);
    TEST_ASSERT_STR(h, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    sha256_hex((const uint8_t *)"", 0, h);
    TEST_ASSERT_STR(h, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    const char *m = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    sha256_hex((const uint8_t *)m, strlen(m), h);
    TEST_ASSERT_STR(h, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

static void test_login_ok_and_fail(void)
{
    auth_reset();
    TEST_ASSERT_EQ(auth_check("admin", "Test#Pass1", 1000), AUTH_OK);
    TEST_ASSERT_EQ(auth_check("admin", "wrong", 1000), AUTH_FAIL);
    TEST_ASSERT_EQ(auth_check("root", "Test#Pass1", 1000), AUTH_FAIL);
}

static void test_lockout_after_5_failures(void)
{
    auth_reset();
    for (int i = 0; i < AUTH_MAX_FAILS - 1; i++)
        TEST_ASSERT_EQ(auth_check("admin", "x", 2000), AUTH_FAIL);
    TEST_ASSERT_EQ(auth_check("admin", "x", 2000), AUTH_LOCKED);
    TEST_ASSERT_EQ(auth_check("admin", "Test#Pass1", 2001), AUTH_LOCKED); /* even correct pw */
    TEST_ASSERT_EQ(auth_check("admin", "Test#Pass1", 2000 + AUTH_LOCK_SECONDS), AUTH_OK);
}

static void test_base64(void)
{
    char out[64];
    TEST_ASSERT_EQ(base64_decode("YWRtaW46VGVzdCNQYXNzMQ==", out, sizeof out), 16);
    TEST_ASSERT_STR(out, "admin:Test#Pass1");
    TEST_ASSERT_EQ(base64_decode("@@@", out, sizeof out), -1);
}

static void test_audit_records_logins(void)
{
    char buf[4096];
    TEST_ASSERT(audit_dump_json(buf, sizeof buf) > 0);
    TEST_ASSERT(strstr(buf, "\"action\":\"login\"") != NULL);
}

int main(void)
{
    test_config();
    RUN_TEST(test_sha256_vectors);
    RUN_TEST(test_login_ok_and_fail);
    RUN_TEST(test_lockout_after_5_failures);
    RUN_TEST(test_base64);
    RUN_TEST(test_audit_records_logins);
    return TEST_END();
}
