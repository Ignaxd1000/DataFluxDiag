#ifndef STREAMDIAG_TRANSFER_H
#define STREAMDIAG_TRANSFER_H

#include <stdint.h>

/* Transfer protocol constants and binary header. */
#define SD_TRANSFER_MAGIC 0x53445446u /* 'SDTF' */
#define SD_TRANSFER_VERSION 1u

typedef struct sd_transfer_header {
    uint32_t magic;
    uint16_t version;
    uint16_t filename_length;
    uint64_t file_size;
} sd_transfer_header;

/* Starts file receiver server. */
int sd_transfer_run_server(uint16_t port);
/* Sends one file to host:port receiver. */
int sd_transfer_send_file(const char *host, uint16_t port, const char *file_path);

#endif
