#include "platform/platform.h"

#include "platform/socket.h"

#include <time.h>

int sd_platform_init(void) {
    return sd_socket_global_init();
}

void sd_platform_cleanup(void) {
    sd_socket_global_cleanup();
}

double sd_platform_now_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0.0;
    }
    return ((double)ts.tv_sec * 1000.0) + ((double)ts.tv_nsec / 1000000.0);
}
