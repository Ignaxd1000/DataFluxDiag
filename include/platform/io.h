#ifndef STREAMDIAG_PLATFORM_IO_H
#define STREAMDIAG_PLATFORM_IO_H

#include <stddef.h>
#include <stdint.h>

typedef struct sd_file {
    intptr_t native;
} sd_file;

#define SD_FILE_INVALID ((intptr_t)-1)

/* Portable low-level file I/O wrappers used by diagnostics. */
int sd_io_open_read(const char *path, sd_file *file);
int sd_io_open_write(const char *path, sd_file *file);
int sd_io_read(sd_file file, void *buffer, size_t bytes_to_read, size_t *bytes_read);
int sd_io_write(sd_file file, const void *buffer, size_t bytes_to_write, size_t *bytes_written);
int sd_io_get_size(sd_file file, uint64_t *size_out);
int sd_io_close(sd_file *file);

#endif
