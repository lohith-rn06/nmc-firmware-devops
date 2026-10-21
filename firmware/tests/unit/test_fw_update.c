#include "fw_update.h"
#include "test_helpers.h"
#include "unity_lite.h"
#include <stdio.h>
#include <stdlib.h>

static void write_image(const char *path, const char *variant, int corrupt)
{
    uint8_t hdr[FW_HEADER_SIZE] = {0};
    const char payload[] = "pretend this is a squashfs root filesystem";
    uint32_t len = sizeof payload;
    memcpy(hdr, FW_MAGIC, 4);
    hdr[4] = 1;
    strncpy((char *)hdr + 8, variant, 16);
    strncpy((char *)hdr + 24, "1.4.0", 32);
    hdr[56] = (uint8_t)len;
    hdr[57] = (uint8_t)(len >> 8);
    hdr[58] = (uint8_t)(len >> 16);
    hdr[59] = (uint8_t)(len >> 24);
    sha256((const uint8_t *)payload, len, hdr + 60);
    FILE *f = fopen(path, "wb");
    fwrite(hdr, 1, sizeof hdr, f);
    fwrite(payload, 1, len, f);
    if (corrupt) {
        fseek(f, FW_HEADER_SIZE + 3, SEEK_SET);
        fputc('X', f);
    }
    fclose(f);
}

static void test_valid_image(void)
{
    char err[160];
    write_image("t_ok.nmcfw", NMC_MODEL, 0);
    TEST_ASSERT_EQ(fw_validate_file("t_ok.nmcfw", err, sizeof err), 0);
    remove("t_ok.nmcfw");
}

static void test_wrong_variant_rejected(void)
{
    char err[160];
    write_image("t_var.nmcfw", strcmp(NMC_MODEL, "GVS") ? "GVS" : "GVXL", 0);
    TEST_ASSERT_EQ(fw_validate_file("t_var.nmcfw", err, sizeof err), -1);
    TEST_ASSERT(strstr(err, "variant mismatch") != NULL);
    remove("t_var.nmcfw");
}

static void test_corrupted_rejected(void)
{
    char err[160];
    write_image("t_bad.nmcfw", NMC_MODEL, 1);
    TEST_ASSERT_EQ(fw_validate_file("t_bad.nmcfw", err, sizeof err), -1);
    TEST_ASSERT(strstr(err, "sha256") != NULL);
    remove("t_bad.nmcfw");
}

int main(void)
{
    test_config();
    RUN_TEST(test_valid_image);
    RUN_TEST(test_wrong_variant_rejected);
    RUN_TEST(test_corrupted_rejected);
    return TEST_END();
}
