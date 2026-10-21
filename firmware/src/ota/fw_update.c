/* Firmware image validation run on the card before flashing:
 * magic, header version, variant match (a GVS image must never flash a GVXL card),
 * payload length and SHA-256 integrity. Authenticity (signature) is checked separately. */
#include "fw_update.h"
#include "nmc.h"
#include "security.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FAIL(...)                                                                                                                \
    do {                                                                                                                         \
        snprintf(err, errlen, __VA_ARGS__);                                                                                      \
        goto out;                                                                                                                \
    } while (0)

int fw_validate_file(const char *path, char *err, size_t errlen)
{
    int rc = -1;
    uint8_t *payload = NULL;
    uint8_t hdr[FW_HEADER_SIZE];
    FILE *f = fopen(path, "rb");
    if (!f) {
        snprintf(err, errlen, "cannot open %s", path);
        return -1;
    }
    if (fread(hdr, 1, sizeof hdr, f) != sizeof hdr) FAIL("short header");
    if (memcmp(hdr, FW_MAGIC, 4) != 0) FAIL("bad magic");
    if (hdr[4] != 1) FAIL("unsupported header version %u", hdr[4]);
    char variant[17] = {0};
    memcpy(variant, hdr + 8, 16);
    if (strcmp(variant, NMC_MODEL) != 0) FAIL("variant mismatch: image=%s card=%s", variant, NMC_MODEL);
    uint32_t plen = (uint32_t)hdr[56] | (uint32_t)hdr[57] << 8 | (uint32_t)hdr[58] << 16 | (uint32_t)hdr[59] << 24;
    if (plen == 0 || plen > 64u * 1024 * 1024) FAIL("bad payload length %u", plen);
    payload = malloc(plen);
    if (!payload) FAIL("out of memory");
    if (fread(payload, 1, plen, f) != plen) FAIL("truncated payload");
    uint8_t digest[32];
    sha256(payload, plen, digest);
    if (memcmp(digest, hdr + 60, 32) != 0) FAIL("payload sha256 mismatch (corrupted image)");
    snprintf(err, errlen, "ok");
    rc = 0;
out:
    free(payload);
    fclose(f);
    return rc;
}
