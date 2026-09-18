#include "utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sd_validate_buffer_size(size_t buffer_size) {
    return buffer_size > 0 && buffer_size <= SD_MAX_BUFFER_SIZE;
}

static int parse_size_token(const char *token, size_t *out_value) {
    char *end = NULL;
    unsigned long long value = strtoull(token, &end, 10);
    if (token == end || *end != '\0') {
        return -1;
    }
    if (value == 0 || value > SD_MAX_BUFFER_SIZE) {
        return -1;
    }
    *out_value = (size_t)value;
    return 0;
}

int sd_parse_buffer_sizes(const char *text,
                          size_t *out_sizes,
                          size_t max_sizes,
                          size_t *out_count) {
    char *cursor = NULL;
    char *work = NULL;
    size_t count = 0;

    if (!text || !out_sizes || !out_count || max_sizes == 0) {
        return -1;
    }

    work = (char *)malloc(strlen(text) + 1u);
    if (!work) {
        return -1;
    }
    strcpy(work, text);

    cursor = strtok(work, ",");
    while (cursor) {
        char *dash = strchr(cursor, '-');
        if (dash) {
            size_t start = 0;
            size_t end = 0;
            size_t step = 0;
            char *step_sep = strchr(dash + 1, ':');
            *dash = '\0';
            if (parse_size_token(cursor, &start) != 0) {
                free(work);
                return -1;
            }
            if (step_sep) {
                *step_sep = '\0';
                if (parse_size_token(step_sep + 1, &step) != 0) {
                    free(work);
                    return -1;
                }
            } else {
                step = start;
            }
            if (parse_size_token(dash + 1, &end) != 0 || start > end || step == 0) {
                free(work);
                return -1;
            }
            for (size_t current = start; current <= end; current += step) {
                if (!sd_validate_buffer_size(current) || count >= max_sizes) {
                    free(work);
                    return -1;
                }
                out_sizes[count++] = current;
                if (end - current < step) {
                    break;
                }
            }
        } else {
            size_t value = 0;
            if (parse_size_token(cursor, &value) != 0 || count >= max_sizes) {
                free(work);
                return -1;
            }
            out_sizes[count++] = value;
        }
        cursor = strtok(NULL, ",");
    }

    if (count == 0) {
        free(work);
        return -1;
    }

    *out_count = count;
    free(work);
    return 0;
}

int sd_parse_port_range(const char *text, uint16_t *start_port, uint16_t *end_port) {
    unsigned long start = 0;
    unsigned long end = 0;

    if (!text || !start_port || !end_port) {
        return -1;
    }

    if (sscanf(text, "%lu-%lu", &start, &end) != 2) {
        return -1;
    }

    if (start == 0 || end == 0 || start > 65535 || end > 65535 || start > end) {
        return -1;
    }

    *start_port = (uint16_t)start;
    *end_port = (uint16_t)end;
    return 0;
}

int sd_parse_host_port(const char *text,
                       char *host_out,
                       size_t host_out_size,
                       uint16_t *port_out) {
    const char *colon = NULL;
    size_t host_len = 0;
    unsigned long port = 0;

    if (!text || !host_out || host_out_size == 0 || !port_out) {
        return -1;
    }

    colon = strrchr(text, ':');
    if (!colon || colon == text) {
        return -1;
    }

    host_len = (size_t)(colon - text);
    if (host_len >= host_out_size) {
        return -1;
    }

    memcpy(host_out, text, host_len);
    host_out[host_len] = '\0';

    port = strtoul(colon + 1, NULL, 10);
    if (port == 0 || port > 65535) {
        return -1;
    }

    *port_out = (uint16_t)port;
    return 0;
}

double sd_calculate_throughput_mib_per_s(uint64_t bytes, double elapsed_ms) {
    const double bytes_per_mib = 1024.0 * 1024.0;
    if (elapsed_ms <= 0.0) {
        return 0.0;
    }
    return ((double)bytes / bytes_per_mib) / (elapsed_ms / 1000.0);
}

uint32_t sd_checksum32(const uint8_t *data, size_t len) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

const char *sd_basename_from_path(const char *path) {
    const char *base = path;
    if (!path) {
        return "";
    }
    for (const char *c = path; *c; ++c) {
        if (*c == '/' || *c == '\\') {
            base = c + 1;
        }
    }
    return base;
}

int sd_parse_positive_int(const char *text, int fallback_value) {
    long value = 0;
    if (!text) {
        return fallback_value;
    }
    value = strtol(text, NULL, 10);
    if (value <= 0 || value > 1000000) {
        return fallback_value;
    }
    return (int)value;
}
