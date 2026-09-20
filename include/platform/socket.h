#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct sd_socket {
    intptr_t native;
} sd_socket;

#define SD_SOCKET_INVALID ((intptr_t)-1)

/* API portable para sockets TCP IPv4 */
int sd_socket_global_init(void);
void sd_socket_global_cleanup(void);
int sd_socket_resolve_ipv4(const char *host, uint32_t *ipv4_network_order_out);
int sd_socket_create_tcp(sd_socket *socket_out);
int sd_socket_connect_with_timeout(sd_socket *socket,
                                   uint32_t ipv4_network_order,
                                   uint16_t port,
                                   int timeout_ms,
                                   int *timed_out,
                                   double *latency_ms);
int sd_socket_bind_listen(sd_socket *socket_out, uint16_t port, int backlog);
int sd_socket_accept(sd_socket listener, sd_socket *client_out);
int sd_socket_send_all(sd_socket socket, const uint8_t *data, size_t length);
int sd_socket_recv_exact(sd_socket socket, uint8_t *data, size_t length);
int sd_socket_close(sd_socket *socket);

