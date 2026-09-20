#include "scanner.h"
#include "platform/socket.h"
#include <inttypes.h>
#include <stdio.h>

int sd_scan_tcp_ports(const char *host,
                      uint16_t start_port,
                      uint16_t end_port,
                      int timeout_ms) {
    uint32_t ipv4 = 0;

    if (!host || start_port == 0 || end_port == 0 || start_port > end_port || timeout_ms <= 0) {
        fprintf(stderr, "Error: invalid scanner arguments.\n");
        return 1;
    }

    if (sd_socket_resolve_ipv4(host, &ipv4) != 0) {
        fprintf(stderr, "Error: could not resolve host '%s'.\n", host);
        return 1;
    }

    printf("SOCKET SCANNER\n");
    printf("----------------------------------------\n");
    printf("Host: %s\n", host);
    printf("Protocol: TCP (IPv4)\n");
    printf("Ports: %u-%u\n", (unsigned)start_port, (unsigned)end_port);
    printf("Timeout: %d ms\n\n", timeout_ms);
    printf("%-10s %-10s %-10s\n", "PORT", "STATE", "LATENCY");

    for (uint16_t port = start_port; port <= end_port; ++port) {
        sd_socket socket = {SD_SOCKET_INVALID};
        int timed_out = 0;
        double latency_ms = 0.0;
        int rc = 0;

        if (sd_socket_create_tcp(&socket) != 0) {
            fprintf(stderr, "Error: failed to create socket for port %u.\n", (unsigned)port);
            continue;
        }

        rc = sd_socket_connect_with_timeout(&socket, ipv4, port, timeout_ms, &timed_out, &latency_ms);
        if (rc == 0) {
            printf("%-10u %-10s %.3f ms\n", (unsigned)port, "OPEN", latency_ms);
        } else if (timed_out) {
            printf("%-10u %-10s -\n", (unsigned)port, "TIMEOUT");
        } else {
            printf("%-10u %-10s -\n", (unsigned)port, "CLOSED");
        }

        sd_socket_close(&socket);

        if (port == end_port) {
            break;
        }
    }

    return 0;
}
