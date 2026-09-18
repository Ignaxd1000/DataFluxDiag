#ifndef STREAMDIAG_UTILS_H
#define STREAMDIAG_UTILS_H

#include <stddef.h>
#include <stdint.h>

#define SD_DEFAULT_BUFFER_SIZE 65536u
#define SD_MAX_BUFFER_SIZE (64u * 1024u * 1024u)

/* Validates non-zero, bounded buffer size. */
int sd_validate_buffer_size(size_t buffer_size);
/* Parses a comma-separated list and/or inclusive ranges start-end[:step]. */
int sd_parse_buffer_sizes(const char *text,
                          size_t *out_sizes,
                          size_t max_sizes,
                          size_t *out_count);
/* Parses inclusive port range (start-end). */
int sd_parse_port_range(const char *text, uint16_t *start_port, uint16_t *end_port);
/* Parses endpoint host:port. */
int sd_parse_host_port(const char *text,
                       char *host_out,
                       size_t host_out_size,
                       uint16_t *port_out);
/* Calculates throughput in MiB/s. */
double sd_calculate_throughput_mib_per_s(uint64_t bytes, double elapsed_ms);
/* Simple checksum helper used by tests and integrity checks. */
uint32_t sd_checksum32(const uint8_t *data, size_t len);
/* Returns basename pointer inside path string. */
const char *sd_basename_from_path(const char *path);
/* Parses positive int with fallback default. */
int sd_parse_positive_int(const char *text, int fallback_value);

#endif
