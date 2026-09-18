#ifndef STREAMDIAG_PLATFORM_PLATFORM_H
#define STREAMDIAG_PLATFORM_PLATFORM_H

/* Process/platform initialization (timers and sockets support). */
int sd_platform_init(void);
void sd_platform_cleanup(void);
/* Monotonic high-resolution timestamp in milliseconds. */
double sd_platform_now_ms(void);

#endif
