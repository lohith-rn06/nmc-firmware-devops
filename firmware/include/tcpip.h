#ifndef NMC_TCPIP_H
#define NMC_TCPIP_H
int tcp_listen(int port);
int udp_bind(int port);
int tcp_connect_timeout(const char *host, int port, int timeout_s);
#endif
