/* PCNS notifier: when the UPS changes state (e.g. goes on battery) every registered
 * PowerChute Network Shutdown client gets a JSON event so servers can shut down cleanly. */
#include "log.h"
#include "nmc.h"
#include "pcns.h"
#include "tcpip.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

int pcns_format_event(char *buf, size_t cap, ups_status_t s)
{
    ups_data_t d;
    ups_get(&d);
    const char *ev = s == UPS_ON_BATTERY ? "ON_BATTERY" : s == UPS_BYPASS ? "ON_BYPASS" : "POWER_RESTORED";
    const char *action = (s == UPS_ON_BATTERY && d.runtime_min < 5) ? "SHUTDOWN_NOW"
                         : (s == UPS_ON_BATTERY)                    ? "PREPARE_SHUTDOWN"
                                                                    : "NONE";
    return snprintf(buf, cap,
                    "{\"event\":\"%s\",\"model\":\"%s\",\"runtime_min\":%d,\"battery_pct\":%d,"
                    "\"action\":\"%s\",\"ts\":%ld}\n",
                    ev, NMC_MODEL, d.runtime_min, d.battery_pct, action, (long)time(NULL));
}

typedef struct {
    ups_status_t s;
} job_t;

static void *notify_thread(void *arg)
{
    job_t *j = arg;
    char msg[512];
    int n = pcns_format_event(msg, sizeof msg, j->s);
    char list[256];
    snprintf(list, sizeof list, "%s", g_cfg.pcns_clients);
    for (char *save = NULL, *t = strtok_r(list, ",", &save); t; t = strtok_r(NULL, ",", &save)) {
        char *colon = strrchr(t, ':');
        if (!colon) continue;
        *colon = 0;
        char *end;
        long port = strtol(colon + 1, &end, 10);
        if (*end || port < 1 || port > 65535) continue;
        int fd = tcp_connect_timeout(t, (int)port, 2);
        if (fd < 0) {
            nmc_log(LOG_WRN, "pcns", "client %s:%ld unreachable", t, port);
            continue;
        }
        if (send(fd, msg, (size_t)n, MSG_NOSIGNAL) == n) nmc_log(LOG_INF, "pcns", "notified %s:%ld", t, port);
        close(fd);
    }
    free(j);
    return NULL;
}

void pcns_on_status(ups_status_t old_s, ups_status_t new_s)
{
    (void)old_s;
    if (!g_cfg.pcns_clients[0]) return;
    job_t *j = malloc(sizeof *j);
    if (!j) return;
    j->s = new_s;
    pthread_t t;
    if (pthread_create(&t, NULL, notify_thread, j) != 0) {
        free(j);
        return;
    }
    pthread_detach(t);
}

void pcns_init(void)
{
    ups_on_status_change(pcns_on_status);
    nmc_log(LOG_INF, "pcns", "PCNS notifier ready (clients: %s)", g_cfg.pcns_clients[0] ? g_cfg.pcns_clients : "none");
}
