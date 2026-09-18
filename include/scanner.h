#ifndef STREAMDIAG_SCANNER_H
#define STREAMDIAG_SCANNER_H

#include <stdint.h>

/* Sequential TCP IPv4 connect scanner. */
int sd_scan_tcp_ports(const char *host,
                      uint16_t start_port,
                      uint16_t end_port,
                      int timeout_ms);

#endif
