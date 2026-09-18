#include "buffer_diag.h"

#include "platform/io.h"
#include "platform/platform.h"
#include "utils.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int run_single_benchmark(const char *input_path,
                                const char *copy_output_path,
                                size_t buffer_size,
                                uint64_t file_size) {
    sd_file in_file = {SD_FILE_INVALID};
    sd_file out_file = {SD_FILE_INVALID};
    uint8_t *buffer = NULL;
    uint64_t bytes_processed = 0;
    uint64_t operations = 0;
    double started_ms = 0.0;
    double elapsed_ms = 0.0;

    if (!sd_validate_buffer_size(buffer_size)) {
        fprintf(stderr, "Error: invalid buffer size %zu.\n", buffer_size);
        return 1;
    }

    buffer = (uint8_t *)malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Error: cannot allocate %zu bytes buffer.\n", buffer_size);
        return 1;
    }

    if (sd_io_open_read(input_path, &in_file) != 0) {
        fprintf(stderr, "Error: cannot open input file: %s\n", input_path);
        free(buffer);
        return 1;
    }

    if (copy_output_path && sd_io_open_write(copy_output_path, &out_file) != 0) {
        fprintf(stderr, "Error: cannot open output file: %s\n", copy_output_path);
        sd_io_close(&in_file);
        free(buffer);
        return 1;
    }

    started_ms = sd_platform_now_ms();

    while (1) {
        size_t bytes_read = 0;
        if (sd_io_read(in_file, buffer, buffer_size, &bytes_read) != 0) {
            fprintf(stderr, "Error: read operation failed.\n");
            sd_io_close(&in_file);
            sd_io_close(&out_file);
            free(buffer);
            return 1;
        }

        if (bytes_read == 0) {
            break;
        }

        if (copy_output_path) {
            size_t written_total = 0;
            while (written_total < bytes_read) {
                size_t bytes_written = 0;
                if (sd_io_write(out_file,
                                buffer + written_total,
                                bytes_read - written_total,
                                &bytes_written) != 0) {
                    fprintf(stderr, "Error: write operation failed.\n");
                    sd_io_close(&in_file);
                    sd_io_close(&out_file);
                    free(buffer);
                    return 1;
                }
                if (bytes_written == 0) {
                    fprintf(stderr, "Error: write returned zero bytes unexpectedly.\n");
                    sd_io_close(&in_file);
                    sd_io_close(&out_file);
                    free(buffer);
                    return 1;
                }
                written_total += bytes_written;
            }
        }

        bytes_processed += bytes_read;
        operations += 1;
    }

    elapsed_ms = sd_platform_now_ms() - started_ms;

    printf("%-12zu %-14" PRIu64 " %-10.3f %-10.3f\n",
           buffer_size,
           operations,
           elapsed_ms,
           sd_calculate_throughput_mib_per_s(bytes_processed, elapsed_ms));

    if (bytes_processed != file_size) {
        fprintf(stderr,
                "Warning: bytes processed (%" PRIu64 ") differs from expected file size (%" PRIu64 ").\n",
                bytes_processed,
                file_size);
    }

    sd_io_close(&in_file);
    sd_io_close(&out_file);
    free(buffer);
    return 0;
}

int sd_buffer_benchmark(const char *input_path,
                        const char *copy_output_path,
                        const size_t *buffer_sizes,
                        size_t buffer_count) {
    sd_file input = {SD_FILE_INVALID};
    uint64_t size = 0;

    if (!input_path || !buffer_sizes || buffer_count == 0) {
        fprintf(stderr, "Error: missing benchmark arguments.\n");
        return 1;
    }

    if (sd_io_open_read(input_path, &input) != 0 || sd_io_get_size(input, &size) != 0) {
        fprintf(stderr, "Error: cannot read metadata for file: %s\n", input_path);
        sd_io_close(&input);
        return 1;
    }
    sd_io_close(&input);

    printf("BUFFER DIAGNOSTIC\n");
    printf("----------------------------------------\n");
    printf("File: %s\n", input_path);
    printf("Size: %" PRIu64 " bytes\n", size);
    printf("Operation: %s\n\n", copy_output_path ? "COPY" : "READ");
    printf("%-12s %-14s %-10s %-10s\n", "Buffer", "Operations", "Time(ms)", "MiB/s");

    for (size_t i = 0; i < buffer_count; ++i) {
        if (run_single_benchmark(input_path, copy_output_path, buffer_sizes[i], size) != 0) {
            return 1;
        }
    }

    return 0;
}
