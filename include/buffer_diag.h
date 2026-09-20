#pragma once
#include <stddef.h>

/* Ejecuta una prueba de benchmark para leer/copiar un archivo con uno o varios tamaños de buffer. */
int sd_buffer_benchmark(const char *input_path,
                        const char *copy_output_path,
                        const size_t *buffer_sizes,
                        size_t buffer_count);


