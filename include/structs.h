#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int port_number;
    int is_closed;
} port;

typedef struct {
    uint32_t ipv4;
    double latency_ms;
    port port;
    int rc;
    int timed_out;
    int timeout_ms;
} scan_task;    

typedef struct {
    scan_task *tasks;
    size_t count;
} scan_result_list;



