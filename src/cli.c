#include "cli.h"

#include "buffer_diag.h"
#include "platform/platform.h"
#include "scanner.h"
#include "transfer.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(void) {
    printf("StreamDiag - CLI diagnostic tool\n\n");
    printf("Usage:\n");
    printf("  streamdiag --help\n");
    printf("  streamdiag buffer benchmark <file> [--copy-out <file>] [--buffers <list>]\n");
    printf("  streamdiag scan <host> <start-end> [--timeout <ms>]\n");
    printf("  streamdiag server --port <port>\n");
    printf("  streamdiag send <host:port> <file>\n\n");
    printf("Buffer list format examples:\n");
    printf("  512,1024,4096\n");
    printf("  1024-65536:1024\n");
}

static int run_interactive_menu(void) {
    int choice = -1;
    char host[256];
    char range[64];
    char endpoint[256];
    char file[512];
    char buffer_text[256];

    printf("========================================\n");
    printf("             StreamDiag\n");
    printf("========================================\n");
    printf("1. Buffer Diagnostic\n");
    printf("2. TCP Socket Scanner\n");
    printf("3. TCP File Transfer\n");
    printf("0. Exit\n");
    printf("Select: ");

    if (scanf("%d", &choice) != 1) {
        fprintf(stderr, "Error: invalid menu input.\n");
        return 1;
    }

    if (choice == 0) {
        return 0;
    }

    if (choice == 1) {
        size_t sizes[64];
        size_t size_count = 0;
        printf("Input file: ");
        if (scanf("%511s", file) != 1) {
            return 1;
        }
        printf("Buffer sizes (list/range): ");
        if (scanf("%255s", buffer_text) != 1) {
            return 1;
        }
        if (sd_parse_buffer_sizes(buffer_text, sizes, 64, &size_count) != 0) {
            fprintf(stderr, "Error: invalid buffer sizes format.\n");
            return 1;
        }
        return sd_buffer_benchmark(file, NULL, sizes, size_count);
    }

    if (choice == 2) {
        uint16_t start = 0;
        uint16_t end = 0;
        printf("Host: ");
        if (scanf("%255s", host) != 1) {
            return 1;
        }
        printf("Port range (start-end): ");
        if (scanf("%63s", range) != 1) {
            return 1;
        }
        if (sd_parse_port_range(range, &start, &end) != 0) {
            fprintf(stderr, "Error: invalid port range.\n");
            return 1;
        }
        return sd_scan_tcp_ports(host, start, end, 1000);
    }

    if (choice == 3) {
        int mode = 0;
        printf("1=Server 2=Send: ");
        if (scanf("%d", &mode) != 1) {
            return 1;
        }
        if (mode == 1) {
            int port = 0;
            printf("Port: ");
            if (scanf("%d", &port) != 1 || port <= 0 || port > 65535) {
                fprintf(stderr, "Error: invalid port.\n");
                return 1;
            }
            return sd_transfer_run_server((uint16_t)port);
        }
        if (mode == 2) {
            char host_value[256];
            uint16_t port = 0;
            printf("Endpoint host:port: ");
            if (scanf("%255s", endpoint) != 1) {
                return 1;
            }
            if (sd_parse_host_port(endpoint, host_value, sizeof(host_value), &port) != 0) {
                fprintf(stderr, "Error: invalid endpoint.\n");
                return 1;
            }
            printf("File path: ");
            if (scanf("%511s", file) != 1) {
                return 1;
            }
            return sd_transfer_send_file(host_value, port, file);
        }
    }

    fprintf(stderr, "Error: unknown option.\n");
    return 1;
}

int sd_cli_run(int argc, char **argv) {
    if (sd_platform_init() != 0) {
        fprintf(stderr, "Error: failed to initialize platform.\n");
        return 1;
    }

    if (argc == 1) {
        int rc = run_interactive_menu();
        sd_platform_cleanup();
        return rc;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        sd_platform_cleanup();
        return 0;
    }

    if (strcmp(argv[1], "buffer") == 0) {
        size_t sizes[128];
        size_t size_count = 1;
        const char *copy_out = NULL;
        sizes[0] = SD_DEFAULT_BUFFER_SIZE;

        if (argc < 4 || strcmp(argv[2], "benchmark") != 0) {
            fprintf(stderr, "Error: usage: streamdiag buffer benchmark <file> [options]\n");
            sd_platform_cleanup();
            return 1;
        }

        for (int i = 4; i < argc; ++i) {
            if (strcmp(argv[i], "--buffers") == 0 && i + 1 < argc) {
                if (sd_parse_buffer_sizes(argv[++i], sizes, 128, &size_count) != 0) {
                    fprintf(stderr, "Error: invalid buffer list.\n");
                    sd_platform_cleanup();
                    return 1;
                }
            } else if (strcmp(argv[i], "--copy-out") == 0 && i + 1 < argc) {
                copy_out = argv[++i];
            } else {
                fprintf(stderr, "Error: unknown buffer option: %s\n", argv[i]);
                sd_platform_cleanup();
                return 1;
            }
        }

        {
            int rc = sd_buffer_benchmark(argv[3], copy_out, sizes, size_count);
            sd_platform_cleanup();
            return rc;
        }
    }

    if (strcmp(argv[1], "scan") == 0) {
        uint16_t start = 0;
        uint16_t end = 0;
        int timeout_ms = 1000;

        if (argc < 4) {
            fprintf(stderr, "Error: usage: streamdiag scan <host> <start-end> [--timeout <ms>]\n");
            sd_platform_cleanup();
            return 1;
        }

        if (sd_parse_port_range(argv[3], &start, &end) != 0) {
            fprintf(stderr, "Error: invalid port range.\n");
            sd_platform_cleanup();
            return 1;
        }

        for (int i = 4; i < argc; ++i) {
            if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
                timeout_ms = sd_parse_positive_int(argv[++i], 1000);
            } else {
                fprintf(stderr, "Error: unknown scan option: %s\n", argv[i]);
                sd_platform_cleanup();
                return 1;
            }
        }

        {
            int rc = sd_scan_tcp_ports(argv[2], start, end, timeout_ms);
            sd_platform_cleanup();
            return rc;
        }
    }

    if (strcmp(argv[1], "server") == 0) {
        int port = 0;

        if (argc != 4 || strcmp(argv[2], "--port") != 0) {
            fprintf(stderr, "Error: usage: streamdiag server --port <port>\n");
            sd_platform_cleanup();
            return 1;
        }

        port = sd_parse_positive_int(argv[3], -1);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Error: invalid server port.\n");
            sd_platform_cleanup();
            return 1;
        }

        {
            int rc = sd_transfer_run_server((uint16_t)port);
            sd_platform_cleanup();
            return rc;
        }
    }

    if (strcmp(argv[1], "send") == 0) {
        char host[256];
        uint16_t port = 0;

        if (argc != 4) {
            fprintf(stderr, "Error: usage: streamdiag send <host:port> <file>\n");
            sd_platform_cleanup();
            return 1;
        }

        if (sd_parse_host_port(argv[2], host, sizeof(host), &port) != 0) {
            fprintf(stderr, "Error: invalid endpoint %s\n", argv[2]);
            sd_platform_cleanup();
            return 1;
        }

        {
            int rc = sd_transfer_send_file(host, port, argv[3]);
            sd_platform_cleanup();
            return rc;
        }
    }

    fprintf(stderr, "Error: unknown command. Use --help for usage.\n");
    sd_platform_cleanup();
    return 1;
}
