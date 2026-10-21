/* Embedded web server: status/config REST API + security headers.
 * On the card this sits behind TLS (HTTPS on 443); the simulator speaks plain HTTP
 * and TLS is terminated by the Kubernetes ingress. */
#include "web_server.h"
#include "log.h"
#include "nmc.h"
#include "security.h"
#include "tcpip.h"
#include "ups_model.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define REQ_MAX 4096

static void send_resp(int c, int code, const char *reason, const char *ctype, const char *body, const char *extra)
{
    char hdr[768];
    size_t bl = strlen(body);
    int n = snprintf(hdr, sizeof hdr,
                     "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nConnection: close\r\n"
                     "X-Content-Type-Options: nosniff\r\nX-Frame-Options: DENY\r\n"
                     "Content-Security-Policy: default-src 'self'\r\nCache-Control: no-store\r\n"
                     "Server: NMC\r\n%s\r\n",
                     code, reason, ctype, bl, extra ? extra : "");
    if (n > 0) send(c, hdr, (size_t)n, MSG_NOSIGNAL);
    send(c, body, bl, MSG_NOSIGNAL);
}

static void features_json(char *out, size_t cap)
{
    char tmp[256];
    snprintf(tmp, sizeof tmp, "%s", nmc_features());
    size_t o = (size_t)snprintf(out, cap, "[");
    int first = 1;
    for (char *save = NULL, *t = strtok_r(tmp, ",", &save); t && o < cap; t = strtok_r(NULL, ",", &save)) {
        o += (size_t)snprintf(out + o, cap - o, "%s\"%s\"", first ? "" : ",", t);
        first = 0;
    }
    if (o < cap) snprintf(out + o, cap - o, "]");
}

static int authorized(const char *req, char *who, size_t wcap)
{
    const char *h = strcasestr(req, "\r\nAuthorization: Basic ");
    snprintf(who, wcap, "anonymous");
    if (!h) return AUTH_FAIL;
    h += strlen("\r\nAuthorization: Basic ");
    char b64[256];
    size_t i = 0;
    while (h[i] && h[i] != '\r' && i + 1 < sizeof b64) {
        b64[i] = h[i];
        i++;
    }
    b64[i] = 0;
    char dec[192];
    if (base64_decode(b64, dec, sizeof dec) < 0) return AUTH_FAIL;
    char *colon = strchr(dec, ':');
    if (!colon) return AUTH_FAIL;
    *colon = 0;
    snprintf(who, wcap, "%.31s", dec);
    int rc = auth_check(dec, colon + 1, (long)time(NULL));
    memset(dec, 0, sizeof dec);
    return rc;
}

static void handle(int c)
{
    char req[REQ_MAX + 1];
    size_t got = 0;
    while (got < REQ_MAX) {
        ssize_t n = recv(c, req + got, REQ_MAX - got, 0);
        if (n <= 0) break;
        got += (size_t)n;
        req[got] = 0;
        if (strstr(req, "\r\n\r\n")) break;
    }
    req[got] = 0;
    char method[8] = "", path[128] = "";
    if (sscanf(req, "%7s %127s", method, path) != 2) {
        send_resp(c, 400, "Bad Request", "text/plain", "bad request\n", NULL);
        return;
    }

    char body[4096], who[64];
    if (!strcmp(method, "GET") && !strcmp(path, "/healthz")) {
        send_resp(c, 200, "OK", "text/plain", "ok\n", NULL);
        return;
    }

    if (!strcmp(method, "GET") && !strcmp(path, "/api/version")) {
        char f[300];
        features_json(f, sizeof f);
        snprintf(body, sizeof body, "{\"model\":\"%s\",\"version\":\"%s\",\"git\":\"%s\",\"features\":%s}", NMC_MODEL,
                 NMC_VERSION, NMC_GIT_SHA, f);
        send_resp(c, 200, "OK", "application/json", body, NULL);
        return;
    }
    if (!strcmp(method, "GET") && !strcmp(path, "/api/status")) {
        ups_data_t d;
        ups_get(&d);
        snprintf(body, sizeof body,
                 "{\"model\":\"%s\",\"status\":\"%s\",\"input_voltage\":[%.1f,%.1f,%.1f],\"output_voltage\":[%.1f,%.1f,%.1f],"
                 "\"output_current\":[%.1f,%.1f,%.1f],\"input_frequency\":%.2f,\"output_frequency\":%.2f,\"load_pct\":%d,"
                 "\"battery_pct\":%d,\"runtime_min\":%d,\"battery_temp_c\":%.1f,\"uptime_s\":%u,\"rated_kva\":%d}",
                 NMC_MODEL, ups_status_str(d.status), d.in_v[0], d.in_v[1], d.in_v[2], d.out_v[0], d.out_v[1], d.out_v[2],
                 d.out_a[0], d.out_a[1], d.out_a[2], d.in_freq, d.out_freq, d.load_pct, d.battery_pct, d.runtime_min,
                 d.battery_temp_c, d.uptime_s, NMC_RATED_KVA);
        send_resp(c, 200, "OK", "application/json", body, NULL);
        return;
    }

    /* ---- everything below requires authentication ---- */
    int a = authorized(req, who, sizeof who);
    if (a == AUTH_LOCKED) {
        send_resp(c, 429, "Too Many Requests", "application/json", "{\"error\":\"account locked\"}", "Retry-After: 60\r\n");
        return;
    }
    if (a != AUTH_OK) {
        send_resp(c, 401, "Unauthorized", "application/json", "{\"error\":\"unauthorized\"}",
                  "WWW-Authenticate: Basic realm=\"NMC\"\r\n");
        return;
    }

    if (!strcmp(method, "GET") && !strcmp(path, "/api/config")) {
        snprintf(body, sizeof body,
                 "{\"snmp_port\":%d,\"modbus_port\":%d,\"http_port\":%d,\"modbus_write_enabled\":%s,"
                 "\"pcns_clients\":\"%s\",\"admin_user\":\"%s\"}",
                 g_cfg.snmp_port, g_cfg.modbus_port, g_cfg.http_port, g_cfg.modbus_write_enabled ? "true" : "false",
                 g_cfg.pcns_clients, g_cfg.admin_user);
        send_resp(c, 200, "OK", "application/json", body, NULL);
        return; /* secrets are never returned */
    }
    if (!strcmp(method, "GET") && !strcmp(path, "/api/audit")) {
        if (audit_dump_json(body, sizeof body) < 0) snprintf(body, sizeof body, "[]");
        send_resp(c, 200, "OK", "application/json", body, NULL);
        return;
    }
    if (!strcmp(method, "POST") && !strncmp(path, "/api/simulate/", 14)) {
        if (!g_cfg.simulation) {
            send_resp(c, 403, "Forbidden", "application/json", "{\"error\":\"simulation disabled\"}", NULL);
            return;
        }
        const char *what = path + 14;
        ups_status_t s;
        if (!strcmp(what, "onbattery"))
            s = UPS_ON_BATTERY;
        else if (!strcmp(what, "online"))
            s = UPS_ONLINE;
        else if (!strcmp(what, "bypass"))
            s = UPS_BYPASS;
        else {
            send_resp(c, 404, "Not Found", "application/json", "{\"error\":\"unknown event\"}", NULL);
            return;
        }
        ups_set_status(s);
        audit_log(who, "simulate", what);
        snprintf(body, sizeof body, "{\"status\":\"%s\"}", ups_status_str(s));
        send_resp(c, 200, "OK", "application/json", body, NULL);
        return;
    }
    send_resp(c, 404, "Not Found", "application/json", "{\"error\":\"not found\"}", NULL);
}

static void *http_thread(void *arg)
{
    int fd = (int)(intptr_t)arg;
    for (;;) {
        int c = accept(fd, NULL, NULL);
        if (c < 0) continue;
        struct timeval tv = {.tv_sec = 5};
        setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
        handle(c);
        close(c);
    }
    return NULL;
}

int http_start(void)
{
    int fd = tcp_listen(g_cfg.http_port);
    if (fd < 0) return -1;
    pthread_t t;
    if (pthread_create(&t, NULL, http_thread, (void *)(intptr_t)fd) != 0) return -1;
    pthread_detach(t);
    nmc_log(LOG_INF, "http", "web/REST listening on tcp/%d", g_cfg.http_port);
    return 0;
}
