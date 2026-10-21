/* TCP/IP helpers: IPv4/IPv6 dual-stack listeners and outbound connect with timeout. */
#include "tcpip.h"
#include "log.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int make_socket(int type, int port)
{
    int fd = socket(AF_INET6, type, 0);
    int fam6 = 1;
    if (fd < 0) {
        fd = socket(AF_INET, type, 0);
        fam6 = 0;
    }
    if (fd < 0) return -1;
    int one = 1, zero = 0;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    if (fam6) {
        setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &zero, sizeof zero);
        struct sockaddr_in6 a;
        memset(&a, 0, sizeof a);
        a.sin6_family = AF_INET6;
        a.sin6_addr = in6addr_any;
        a.sin6_port = htons((uint16_t)port);
        if (bind(fd, (struct sockaddr *)&a, sizeof a) == 0) return fd;
    } else {
        struct sockaddr_in a;
        memset(&a, 0, sizeof a);
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_ANY);
        a.sin_port = htons((uint16_t)port);
        if (bind(fd, (struct sockaddr *)&a, sizeof a) == 0) return fd;
    }
    nmc_log(LOG_ERR, "tcpip", "bind port %d failed: %s", port, strerror(errno));
    close(fd);
    return -1;
}

int tcp_listen(int port)
{
    int fd = make_socket(SOCK_STREAM, port);
    if (fd >= 0 && listen(fd, 16) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int udp_bind(int port) { return make_socket(SOCK_DGRAM, port); }

int tcp_connect_timeout(const char *host, int port, int timeout_s)
{
    char ps[8];
    snprintf(ps, sizeof ps, "%d", port);
    struct addrinfo hints = {.ai_socktype = SOCK_STREAM}, *res = NULL;
    if (getaddrinfo(host, ps, &hints, &res) != 0) return -1;
    int fd = -1;
    for (struct addrinfo *r = res; r; r = r->ai_next) {
        fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
        if (fd < 0) continue;
        int fl = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, fl | O_NONBLOCK);
        int rc = connect(fd, r->ai_addr, r->ai_addrlen);
        if (rc < 0 && errno == EINPROGRESS) {
            struct pollfd p = {.fd = fd, .events = POLLOUT};
            int err = 0;
            socklen_t el = sizeof err;
            if (poll(&p, 1, timeout_s * 1000) == 1 && getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &el) == 0 && err == 0) rc = 0;
        }
        if (rc == 0) {
            fcntl(fd, F_SETFL, fl);
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    return fd;
}
