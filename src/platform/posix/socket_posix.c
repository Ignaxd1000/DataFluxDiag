#include "platform/socket.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "platform/platform.h"

int sd_socket_global_init(void) {
    return 0;
}

void sd_socket_global_cleanup(void) {
}

int sd_socket_resolve_ipv4(const char *host, uint32_t *ipv4_network_order_out) {
    struct addrinfo hints;
    struct addrinfo *results = NULL;
    int rc = 0;

    if (!host || !ipv4_network_order_out) {
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    rc = getaddrinfo(host, NULL, &hints, &results);
    if (rc != 0 || !results) {
        return -1;
    }

    *ipv4_network_order_out = ((struct sockaddr_in *)results->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(results);
    return 0;
}

int sd_socket_create_tcp(sd_socket *socket_out) {
    int fd = -1;
    if (!socket_out) {
        return -1;
    }
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    socket_out->native = (intptr_t)fd;
    return 0;
}

int sd_socket_connect_with_timeout(sd_socket *socket,
                                   uint32_t ipv4_network_order,
                                   uint16_t port,
                                   int timeout_ms,
                                   int *timed_out,
                                   double *latency_ms) {
    struct sockaddr_in addr;
    int fd = -1;
    int flags = 0;
    int rc = 0;
    fd_set write_set;
    struct timeval tv;
    double start_ms = 0.0;

    if (!socket || socket->native == SD_SOCKET_INVALID || port == 0 || timeout_ms <= 0 || !timed_out ||
        !latency_ms) {
        return -1;
    }

    *timed_out = 0;
    *latency_ms = 0.0;
    fd = (int)socket->native;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ipv4_network_order;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
        return -1;
    }

    start_ms = sd_platform_now_ms();
    rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == 0) {
        (void)fcntl(fd, F_SETFL, flags);
        *latency_ms = sd_platform_now_ms() - start_ms;
        return 0;
    }

    if (errno != EINPROGRESS) {
        (void)fcntl(fd, F_SETFL, flags);
        return -1;
    }

    FD_ZERO(&write_set);
    FD_SET(fd, &write_set);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    rc = select(fd + 1, NULL, &write_set, NULL, &tv);
    if (rc == 0) {
        *timed_out = 1;
        (void)fcntl(fd, F_SETFL, flags);
        return -1;
    }

    if (rc < 0) {
        (void)fcntl(fd, F_SETFL, flags);
        return -1;
    }

    {
        int so_error = 0;
        socklen_t len = (socklen_t)sizeof(so_error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len) != 0 || so_error != 0) {
            (void)fcntl(fd, F_SETFL, flags);
            return -1;
        }
    }

    (void)fcntl(fd, F_SETFL, flags);
    *latency_ms = sd_platform_now_ms() - start_ms;
    return 0;
}

int sd_socket_bind_listen(sd_socket *socket_out, uint16_t port, int backlog) {
    sd_socket socket;
    struct sockaddr_in addr;
    int yes = 1;

    if (!socket_out || port == 0 || backlog <= 0) {
        return -1;
    }

    if (sd_socket_create_tcp(&socket) != 0) {
        return -1;
    }

    if (setsockopt((int)socket.native, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != 0) {
        sd_socket_close(&socket);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind((int)socket.native, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
        listen((int)socket.native, backlog) != 0) {
        sd_socket_close(&socket);
        return -1;
    }

    *socket_out = socket;
    return 0;
}

int sd_socket_accept(sd_socket listener, sd_socket *client_out) {
    int fd = -1;
    if (listener.native == SD_SOCKET_INVALID || !client_out) {
        return -1;
    }
    fd = accept((int)listener.native, NULL, NULL);
    if (fd < 0) {
        return -1;
    }
    client_out->native = (intptr_t)fd;
    return 0;
}

int sd_socket_send_all(sd_socket socket, const uint8_t *data, size_t length) {
    size_t sent = 0;
    while (sent < length) {
        ssize_t rc = send((int)socket.native, data + sent, length - sent, 0);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (rc == 0) {
            return -1;
        }
        sent += (size_t)rc;
    }
    return 0;
}

int sd_socket_recv_exact(sd_socket socket, uint8_t *data, size_t length) {
    size_t received = 0;
    while (received < length) {
        ssize_t rc = recv((int)socket.native, data + received, length - received, 0);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (rc == 0) {
            return -1;
        }
        received += (size_t)rc;
    }
    return 0;
}

int sd_socket_close(sd_socket *socket) {
    if (!socket || socket->native == SD_SOCKET_INVALID) {
        return 0;
    }
    if (close((int)socket->native) != 0) {
        return -1;
    }
    socket->native = SD_SOCKET_INVALID;
    return 0;
}
