#include "utils.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int test_buffer_validation(void) {
    if (!sd_validate_buffer_size(1) || !sd_validate_buffer_size(65536) ||
        sd_validate_buffer_size(0) || sd_validate_buffer_size(SD_MAX_BUFFER_SIZE + 1u)) {
        return 1;
    }
    return 0;
}

static int test_port_range_parsing(void) {
    uint16_t s = 0;
    uint16_t e = 0;
    if (sd_parse_port_range("20-80", &s, &e) != 0 || s != 20 || e != 80) {
        return 1;
    }
    if (sd_parse_port_range("80-20", &s, &e) == 0) {
        return 1;
    }
    return 0;
}

static int test_host_port_parsing(void) {
    char host[64];
    uint16_t port = 0;
    if (sd_parse_host_port("127.0.0.1:5000", host, sizeof(host), &port) != 0) {
        return 1;
    }
    if (strcmp(host, "127.0.0.1") != 0 || port != 5000) {
        return 1;
    }
    if (sd_parse_host_port("invalid", host, sizeof(host), &port) == 0) {
        return 1;
    }
    return 0;
}

static int test_throughput_calculation(void) {
    double throughput = sd_calculate_throughput_mib_per_s(1048576u, 1000.0);
    if (fabs(throughput - 1.0) > 1e-9) {
        return 1;
    }
    return 0;
}

static int test_checksum_stability(void) {
    const uint8_t bytes[] = {1u, 2u, 3u, 4u, 5u};
    if (sd_checksum32(bytes, sizeof(bytes)) != sd_checksum32(bytes, sizeof(bytes))) {
        return 1;
    }
    return 0;
}

static int test_buffer_size_list_parsing(void) {
    size_t sizes[16];
    size_t count = 0;
    if (sd_parse_buffer_sizes("1024,2048,4096", sizes, 16, &count) != 0 || count != 3 ||
        sizes[0] != 1024 || sizes[2] != 4096) {
        return 1;
    }
    if (sd_parse_buffer_sizes("1024-4096:1024", sizes, 16, &count) != 0 || count != 4 ||
        sizes[3] != 4096) {
        return 1;
    }
    if (sd_parse_buffer_sizes("0", sizes, 16, &count) == 0) {
        return 1;
    }
    return 0;
}

int main(void) {
    int failed = 0;

    failed += test_buffer_validation();
    failed += test_port_range_parsing();
    failed += test_host_port_parsing();
    failed += test_throughput_calculation();
    failed += test_checksum_stability();
    failed += test_buffer_size_list_parsing();

    if (failed != 0) {
        fprintf(stderr, "Tests failed: %d\n", failed);
        return 1;
    }

    printf("All StreamDiag offline tests passed.\n");
    return 0;
}
