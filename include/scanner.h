#pragma once
#include <stdint.h>

/* Scanner de puertos TCP secuencial IPv4 (es re lerdo porque no le puse threading pero bueno) */
int sd_scan_tcp_ports(const char *host,
                      uint16_t start_port,
                      uint16_t end_port,
                      int timeout_ms);
