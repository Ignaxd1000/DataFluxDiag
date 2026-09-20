#include "scanner.h"
#include "platform/socket.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include "structs.h"
#include "utils.h"


static int scan_worker(void *arg)
{
    scan_task *task = arg;

    sd_socket socket = {SD_SOCKET_INVALID};

    int timed_out = 0;
    double latency_ms = 0.0;
    int timeout_ms = 1000;

    if (sd_socket_create_tcp(&socket) != 0) {
        task->rc = -1;
        task->port.is_closed = 1;
        return -1;
    }

    task->rc = sd_socket_connect_with_timeout(
        &socket,
        task->ipv4,
        task->port.port_number,
        timeout_ms,
        &timed_out,
        &latency_ms
    );

    task->timed_out = timed_out;
    task->latency_ms = latency_ms;

    if (task->rc == 0) {
        task->port.is_closed = 0;   
    } else {
        task->port.is_closed = 1;   
    }

    sd_socket_close(&socket);
    return 0;
}

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
    
    size_t count = (size_t)end_port - start_port + 1;
    scan_task *tasks = malloc(sizeof(scan_task) * count);
    thrd_t *threads = malloc(sizeof(thrd_t) * count);

    for (size_t i = 0; i < count; ++i) {
    tasks[i].ipv4 = ipv4;
    tasks[i].port.port_number = start_port + i;
    tasks[i].port.is_closed = 0;
    tasks[i].latency_ms = 0.0;
    tasks[i].rc = 0;
    tasks[i].timed_out = 0;

    thrd_create(
        &threads[i],
        scan_worker,
        &tasks[i]
    );
    }

    for (size_t i = 0; i < count; ++i) {
        thrd_join(threads[i], NULL);
    }
    qsort(tasks, count, sizeof(scan_task), compare_ports); 
    for (size_t i = 0; i < count; ++i) {
    const char *state = "OPEN";

    if (tasks[i].timed_out) {
        state = "TIMEOUT";
    } else if (tasks[i].port.is_closed) {
        state = "CLOSED";
    }

    printf("%-10u %-10s %.3f ms\n",
           (unsigned)tasks[i].port.port_number,
           state,
           tasks[i].latency_ms);
    }

    /** 
    if (rc == 0) {
            printf("%-10u %-10s %.3f ms\n", (unsigned)port, "OPEN", latency_ms);
        } else if (timed_out) {
            printf("%-10u %-10s -\n", (unsigned)port, "TIMEOUT");
        } else {
            printf("%-10u %-10s -\n", (unsigned)port, "CLOSED");
        }
        **/
    return 0;
}
