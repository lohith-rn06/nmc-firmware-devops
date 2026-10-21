/* Modbus TCP server (port 502 on the card, 1502 in the simulator).
 * Supports FC03 Read Holding Registers, FC04 Read Input Registers, FC06 Write Single
 * Register (rejected unless NMC_MODBUS_WRITE=1 - secure default). */
#include "modbus_tcp.h"
#include "log.h"
#include "nmc.h"
#include "security.h"
#include "tcpip.h"
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define EX_ILLEGAL_FUNCTION 0x01
#define EX_ILLEGAL_ADDRESS 0x02
#define EX_ILLEGAL_VALUE 0x03

static size_t exception(const uint8_t *req, uint8_t fc, uint8_t code, uint8_t *resp, size_t cap)
{
    if (cap < 9) return 0;
    memcpy(resp, req, 4); /* transaction id + protocol id */
    resp[4] = 0;
    resp[5] = 3; /* length = unit + fc + code */
    resp[6] = req[6];
    resp[7] = fc | 0x80;
    resp[8] = code;
    return 9;
}

size_t modbus_process(const uint8_t *req, size_t len, uint8_t *resp, size_t cap)
{
    if (len < 8) return 0;
    uint16_t proto = (uint16_t)(req[2] << 8 | req[3]);
    uint16_t mlen = (uint16_t)(req[4] << 8 | req[5]);
    if (proto != 0 || mlen < 2 || (size_t)mlen + 6 != len) return 0; /* malformed: drop */
    uint8_t fc = req[7];

    if (fc == 0x03 || fc == 0x04) {
        if (len != 12) return exception(req, fc, EX_ILLEGAL_VALUE, resp, cap);
        uint16_t addr = (uint16_t)(req[8] << 8 | req[9]);
        uint16_t qty = (uint16_t)(req[10] << 8 | req[11]);
        if (qty < 1 || qty > 125) return exception(req, fc, EX_ILLEGAL_VALUE, resp, cap);
        if ((uint32_t)addr + qty > MB_REG_COUNT) return exception(req, fc, EX_ILLEGAL_ADDRESS, resp, cap);
        size_t out = 9 + 2u * qty;
        if (cap < out) return 0;
        uint16_t regs[MB_REG_COUNT];
        modbus_fill_registers(regs);
        memcpy(resp, req, 4);
        resp[4] = (uint8_t)((3 + 2 * qty) >> 8);
        resp[5] = (uint8_t)(3 + 2 * qty);
        resp[6] = req[6];
        resp[7] = fc;
        resp[8] = (uint8_t)(2 * qty);
        for (uint16_t i = 0; i < qty; i++) {
            resp[9 + 2 * i] = (uint8_t)(regs[addr + i] >> 8);
            resp[10 + 2 * i] = (uint8_t)regs[addr + i];
        }
        return out;
    }
    if (fc == 0x06) {
        if (!g_cfg.modbus_write_enabled) {
            audit_log("modbus", "write-register", "denied (writes disabled)");
            return exception(req, fc, EX_ILLEGAL_FUNCTION, resp, cap);
        }
        /* Writable command registers are product specific; none exposed in the simulator. */
        return exception(req, fc, EX_ILLEGAL_ADDRESS, resp, cap);
    }
    return exception(req, fc, EX_ILLEGAL_FUNCTION, resp, cap);
}

static void *client_thread(void *arg)
{
    int c = (int)(intptr_t)arg;
    struct timeval tv = {.tv_sec = 30};
    setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    uint8_t in[260], out[260];
    for (;;) {
        ssize_t n = recv(c, in, 7, MSG_WAITALL); /* MBAP header */
        if (n != 7) break;
        size_t body = (size_t)(in[4] << 8 | in[5]);
        if (body < 2 || body > 254) break;
        n = recv(c, in + 7, body - 1, MSG_WAITALL);
        if (n != (ssize_t)(body - 1)) break;
        size_t r = modbus_process(in, 6 + body, out, sizeof out);
        if (!r || send(c, out, r, MSG_NOSIGNAL) != (ssize_t)r) break;
    }
    close(c);
    return NULL;
}

static void *accept_thread(void *arg)
{
    int fd = (int)(intptr_t)arg;
    for (;;) {
        int c = accept(fd, NULL, NULL);
        if (c < 0) continue;
        pthread_t t;
        if (pthread_create(&t, NULL, client_thread, (void *)(intptr_t)c) != 0) {
            close(c);
            continue;
        }
        pthread_detach(t);
    }
    return NULL;
}

int modbus_start(void)
{
    int fd = tcp_listen(g_cfg.modbus_port);
    if (fd < 0) return -1;
    pthread_t t;
    if (pthread_create(&t, NULL, accept_thread, (void *)(intptr_t)fd) != 0) return -1;
    pthread_detach(t);
    nmc_log(LOG_INF, "modbus", "Modbus TCP listening on tcp/%d (writes %s)", g_cfg.modbus_port,
            g_cfg.modbus_write_enabled ? "ENABLED" : "disabled");
    return 0;
}
