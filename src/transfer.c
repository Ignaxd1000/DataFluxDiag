#include "transfer.h"

#include "platform/io.h"
#include "platform/platform.h"
#include "platform/socket.h"
#include "utils.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SD_TRANSFER_CHUNK_SIZE 65536u

static uint64_t to_network_u64(uint64_t value) {
    uint64_t result = 0;
    uint8_t *bytes = (uint8_t *)&result;
    bytes[0] = (uint8_t)((value >> 56u) & 0xFFu);
    bytes[1] = (uint8_t)((value >> 48u) & 0xFFu);
    bytes[2] = (uint8_t)((value >> 40u) & 0xFFu);
    bytes[3] = (uint8_t)((value >> 32u) & 0xFFu);
    bytes[4] = (uint8_t)((value >> 24u) & 0xFFu);
    bytes[5] = (uint8_t)((value >> 16u) & 0xFFu);
    bytes[6] = (uint8_t)((value >> 8u) & 0xFFu);
    bytes[7] = (uint8_t)(value & 0xFFu);
    return result;
}

static uint64_t from_network_u64(uint64_t value) {
    const uint8_t *bytes = (const uint8_t *)&value;
    return ((uint64_t)bytes[0] << 56u) | ((uint64_t)bytes[1] << 48u) | ((uint64_t)bytes[2] << 40u) |
           ((uint64_t)bytes[3] << 32u) | ((uint64_t)bytes[4] << 24u) | ((uint64_t)bytes[5] << 16u) |
           ((uint64_t)bytes[6] << 8u) | (uint64_t)bytes[7];
}

static uint16_t to_network_u16(uint16_t value) {
    return (uint16_t)(((value & 0x00FFu) << 8u) | ((value & 0xFF00u) >> 8u));
}

static uint16_t from_network_u16(uint16_t value) {
    return to_network_u16(value);
}

static uint32_t to_network_u32(uint32_t value) {
    return ((value & 0x000000FFu) << 24u) | ((value & 0x0000FF00u) << 8u) |
           ((value & 0x00FF0000u) >> 8u) | ((value & 0xFF000000u) >> 24u);
}

static uint32_t from_network_u32(uint32_t value) {
    return to_network_u32(value);
}

static int send_header_and_name(sd_socket socket,
                                const char *file_name,
                                uint64_t file_size,
                                uint16_t *name_len_out) {
    sd_transfer_header header;
    uint16_t name_len = (uint16_t)strlen(file_name);

    if (name_len == 0 || name_len > 1024) {
        fprintf(stderr, "Error: file name length must be between 1 and 1024 bytes.\n");
        return 1;
    }

    header.magic = to_network_u32(SD_TRANSFER_MAGIC);
    header.version = to_network_u16(SD_TRANSFER_VERSION);
    header.filename_length = to_network_u16(name_len);
    header.file_size = to_network_u64(file_size);

    if (sd_socket_send_all(socket, (const uint8_t *)&header, sizeof(header)) != 0 ||
        sd_socket_send_all(socket, (const uint8_t *)file_name, name_len) != 0) {
        fprintf(stderr, "Error: failed to send transfer header or filename.\n");
        return 1;
    }

    *name_len_out = name_len;
    return 0;
}

int sd_transfer_send_file(const char *host, uint16_t port, const char *file_path) {
    sd_file input = {SD_FILE_INVALID};
    sd_socket socket = {SD_SOCKET_INVALID};
    uint8_t *buffer = NULL;
    uint64_t file_size = 0;
    uint64_t sent_total = 0;
    uint64_t next_report = 10;
    uint32_t ipv4 = 0;
    int timed_out = 0;
    double latency_ms = 0.0;
    uint8_t ack = 0;
    uint16_t name_len = 0;
    const char *base_name = sd_basename_from_path(file_path);
    double started_ms = 0.0;
    double elapsed_ms = 0.0;

    if (!host || port == 0 || !file_path) {
        fprintf(stderr, "Error: invalid send arguments.\n");
        return 1;
    }

    if (sd_io_open_read(file_path, &input) != 0 || sd_io_get_size(input, &file_size) != 0) {
        fprintf(stderr, "Error: cannot open input file '%s'.\n", file_path);
        sd_io_close(&input);
        return 1;
    }

    if (sd_socket_resolve_ipv4(host, &ipv4) != 0 || sd_socket_create_tcp(&socket) != 0) {
        fprintf(stderr, "Error: failed to prepare socket for %s:%u.\n", host, (unsigned)port);
        sd_io_close(&input);
        return 1;
    }

    if (sd_socket_connect_with_timeout(&socket, ipv4, port, 5000, &timed_out, &latency_ms) != 0) {
        fprintf(stderr,
                "Error: could not connect to %s:%u (%s).\n",
                host,
                (unsigned)port,
                timed_out ? "timeout" : "connection error");
        sd_io_close(&input);
        sd_socket_close(&socket);
        return 1;
    }

    if (send_header_and_name(socket, base_name, file_size, &name_len) != 0) {
        sd_io_close(&input);
        sd_socket_close(&socket);
        return 1;
    }

    (void)name_len;

    buffer = (uint8_t *)malloc(SD_TRANSFER_CHUNK_SIZE);
    if (!buffer) {
        fprintf(stderr, "Error: cannot allocate transfer buffer.\n");
        sd_io_close(&input);
        sd_socket_close(&socket);
        return 1;
    }

    started_ms = sd_platform_now_ms();
    while (1) {
        size_t bytes_read = 0;
        if (sd_io_read(input, buffer, SD_TRANSFER_CHUNK_SIZE, &bytes_read) != 0) {
            fprintf(stderr, "Error: failed to read input file data.\n");
            free(buffer);
            sd_io_close(&input);
            sd_socket_close(&socket);
            return 1;
        }
        if (bytes_read == 0) {
            break;
        }
        if (sd_socket_send_all(socket, buffer, bytes_read) != 0) {
            fprintf(stderr, "Error: failed to send file bytes.\n");
            free(buffer);
            sd_io_close(&input);
            sd_socket_close(&socket);
            return 1;
        }
        sent_total += bytes_read;
        if (file_size > 0) {
            uint64_t percentage = (sent_total * 100u) / file_size;
            if (percentage >= next_report) {
                printf("Progress: %" PRIu64 "%%\n", percentage);
                next_report += 10u;
            }
        }
    }
    elapsed_ms = sd_platform_now_ms() - started_ms;

    if (sd_socket_recv_exact(socket, &ack, 1) != 0 || ack != 1u) {
        fprintf(stderr, "Error: transfer acknowledgment failed.\n");
        free(buffer);
        sd_io_close(&input);
        sd_socket_close(&socket);
        return 1;
    }

    printf("Transfer completed\n");
    printf("Connection latency: %.3f ms\n", latency_ms);
    printf("Bytes sent: %" PRIu64 "\n", sent_total);
    printf("Elapsed: %.3f ms\n", elapsed_ms);
    printf("Throughput: %.3f MiB/s\n", sd_calculate_throughput_mib_per_s(sent_total, elapsed_ms));

    free(buffer);
    sd_io_close(&input);
    sd_socket_close(&socket);
    return 0;
}

int sd_transfer_run_server(uint16_t port) {
    sd_socket listener = {SD_SOCKET_INVALID};
    sd_socket client = {SD_SOCKET_INVALID};
    sd_transfer_header header;
    char *filename = NULL;
    char output_path[1200];
    sd_file output = {SD_FILE_INVALID};
    uint8_t *buffer = NULL;
    uint64_t expected_size = 0;
    uint64_t received = 0;
    uint8_t ack = 0;
    double started_ms = 0.0;
    double elapsed_ms = 0.0;

    if (port == 0) {
        fprintf(stderr, "Error: invalid server port.\n");
        return 1;
    }

    if (sd_socket_bind_listen(&listener, port, 1) != 0) {
        fprintf(stderr, "Error: failed to bind/listen on port %u.\n", (unsigned)port);
        return 1;
    }

    printf("Server listening on port %u...\n", (unsigned)port);

    if (sd_socket_accept(listener, &client) != 0) {
        fprintf(stderr, "Error: failed to accept client connection.\n");
        sd_socket_close(&listener);
        return 1;
    }

    if (sd_socket_recv_exact(client, (uint8_t *)&header, sizeof(header)) != 0) {
        fprintf(stderr, "Error: failed to receive transfer header.\n");
        sd_socket_close(&client);
        sd_socket_close(&listener);
        return 1;
    }

    header.magic = from_network_u32(header.magic);
    header.version = from_network_u16(header.version);
    header.filename_length = from_network_u16(header.filename_length);
    header.file_size = from_network_u64(header.file_size);

    if (header.magic != SD_TRANSFER_MAGIC || header.version != SD_TRANSFER_VERSION ||
        header.filename_length == 0 || header.filename_length > 1024) {
        fprintf(stderr, "Error: invalid transfer header values.\n");
        sd_socket_close(&client);
        sd_socket_close(&listener);
        return 1;
    }

    filename = (char *)malloc((size_t)header.filename_length + 1u);
    if (!filename) {
        fprintf(stderr, "Error: failed to allocate filename buffer.\n");
        sd_socket_close(&client);
        sd_socket_close(&listener);
        return 1;
    }

    if (sd_socket_recv_exact(client, (uint8_t *)filename, header.filename_length) != 0) {
        fprintf(stderr, "Error: failed to receive filename.\n");
        free(filename);
        sd_socket_close(&client);
        sd_socket_close(&listener);
        return 1;
    }
    filename[header.filename_length] = '\0';

    snprintf(output_path, sizeof(output_path), "received_%s", sd_basename_from_path(filename));

    if (sd_io_open_write(output_path, &output) != 0) {
        fprintf(stderr, "Error: cannot create output file '%s'.\n", output_path);
        free(filename);
        sd_socket_close(&client);
        sd_socket_close(&listener);
        return 1;
    }

    buffer = (uint8_t *)malloc(SD_TRANSFER_CHUNK_SIZE);
    if (!buffer) {
        fprintf(stderr, "Error: failed to allocate transfer receive buffer.\n");
        sd_io_close(&output);
        free(filename);
        sd_socket_close(&client);
        sd_socket_close(&listener);
        return 1;
    }

    expected_size = header.file_size;
    started_ms = sd_platform_now_ms();

    while (received < expected_size) {
        size_t chunk = SD_TRANSFER_CHUNK_SIZE;
        size_t consumed = 0;

        if (expected_size - received < chunk) {
            chunk = (size_t)(expected_size - received);
        }

        if (sd_socket_recv_exact(client, buffer, chunk) != 0) {
            fprintf(stderr, "Error: failed while receiving file payload.\n");
            free(buffer);
            sd_io_close(&output);
            free(filename);
            sd_socket_close(&client);
            sd_socket_close(&listener);
            return 1;
        }

        while (consumed < chunk) {
            size_t written = 0;
            if (sd_io_write(output, buffer + consumed, chunk - consumed, &written) != 0 || written == 0) {
                fprintf(stderr, "Error: failed while writing output file.\n");
                free(buffer);
                sd_io_close(&output);
                free(filename);
                sd_socket_close(&client);
                sd_socket_close(&listener);
                return 1;
            }
            consumed += written;
        }

        received += chunk;
    }

    elapsed_ms = sd_platform_now_ms() - started_ms;
    ack = (received == expected_size) ? 1u : 0u;
    (void)sd_socket_send_all(client, &ack, 1);

    printf("Received file: %s\n", output_path);
    printf("Expected bytes: %" PRIu64 "\n", expected_size);
    printf("Received bytes: %" PRIu64 "\n", received);
    printf("Elapsed: %.3f ms\n", elapsed_ms);
    printf("Throughput: %.3f MiB/s\n", sd_calculate_throughput_mib_per_s(received, elapsed_ms));

    free(buffer);
    sd_io_close(&output);
    free(filename);
    sd_socket_close(&client);
    sd_socket_close(&listener);
    return (ack == 1u) ? 0 : 1;
}
