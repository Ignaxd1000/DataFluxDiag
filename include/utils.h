#pragma once
#include <stddef.h>
#include <stdint.h>

#define SD_DEFAULT_BUFFER_SIZE 65536u
#define SD_MAX_BUFFER_SIZE (64u * 1024u * 1024u)

/* Valida el tamaño del buffer */
int sd_validate_buffer_size(size_t buffer_size);
/* Analiza una lista de principio a fin */
int sd_parse_buffer_sizes(const char *text,
                          size_t *out_sizes,
                          size_t max_sizes,
                          size_t *out_count);
/* Analiza un rango de puertos start:end */
int sd_parse_port_range(const char *text, uint16_t *start_port, uint16_t *end_port);
/* Analiza un endpoint host:port */
int sd_parse_host_port(const char *text,
                       char *host_out,
                       size_t host_out_size,
                       uint16_t *port_out);
/* Calcula salida en MiB/s */
double sd_calculate_throughput_mib_per_s(uint64_t bytes, double elapsed_ms);
/* Calculador de checksum */
uint32_t sd_checksum32(const uint8_t *data, size_t len);
/* Devuelve un puntero al nombre del archivo desde un path */
const char *sd_basename_from_path(const char *path);
/* Toma una string y devuelve su valor en entero. Si no sirve, devuelve el valor por defecto */
int sd_parse_positive_int(const char *text, int fallback_value);
/* Compara dos puertos para ordenarlos */
int compare_ports(const void *a, const void *b);