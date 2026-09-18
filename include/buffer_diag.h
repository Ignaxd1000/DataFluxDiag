#ifndef STREAMDIAG_BUFFER_DIAG_H
#define STREAMDIAG_BUFFER_DIAG_H

#include <stddef.h>

/* Executes file read/copy benchmark for one or multiple buffer sizes. */
int sd_buffer_benchmark(const char *input_path,
                        const char *copy_output_path,
                        const size_t *buffer_sizes,
                        size_t buffer_count);

#endif
