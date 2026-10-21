/* Firmware image header produced by tools/fwpack.py. Signature is verified separately
 * (detached .sig, ECDSA P-256 / AWS KMS) before the image is offered to the card. */
#ifndef NMC_FW_UPDATE_H
#define NMC_FW_UPDATE_H
#include <stddef.h>
#include <stdint.h>
#define FW_MAGIC "NMCF"
#define FW_HEADER_SIZE 128
typedef struct {
    char magic[4];
    uint8_t header_version;
    uint8_t reserved[3];
    char variant[16];
    char version[32];
    uint32_t payload_len; /* little endian */
    uint8_t payload_sha256[32];
} fw_header_t;
int fw_validate_file(const char *path, char *err, size_t errlen);
#endif
