#include "platform/io.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

int sd_io_open_read(const char *path, sd_file *file) {
    int fd = -1;
    if (!path || !file) {
        return -1;
    }
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    file->native = (intptr_t)fd;
    return 0;
}

int sd_io_open_write(const char *path, sd_file *file) {
    int fd = -1;
    if (!path || !file) {
        return -1;
    }
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }
    file->native = (intptr_t)fd;
    return 0;
}

int sd_io_read(sd_file file, void *buffer, size_t bytes_to_read, size_t *bytes_read) {
    ssize_t rc;
    if (file.native == SD_FILE_INVALID || !buffer || !bytes_read) {
        return -1;
    }
    rc = read((int)file.native, buffer, bytes_to_read);
    if (rc < 0) {
        if (errno == EINTR) {
            return sd_io_read(file, buffer, bytes_to_read, bytes_read);
        }
        return -1;
    }
    *bytes_read = (size_t)rc;
    return 0;
}

int sd_io_write(sd_file file, const void *buffer, size_t bytes_to_write, size_t *bytes_written) {
    ssize_t rc;
    if (file.native == SD_FILE_INVALID || !buffer || !bytes_written) {
        return -1;
    }
    rc = write((int)file.native, buffer, bytes_to_write);
    if (rc < 0) {
        if (errno == EINTR) {
            return sd_io_write(file, buffer, bytes_to_write, bytes_written);
        }
        return -1;
    }
    *bytes_written = (size_t)rc;
    return 0;
}

int sd_io_get_size(sd_file file, uint64_t *size_out) {
    struct stat st;
    if (file.native == SD_FILE_INVALID || !size_out) {
        return -1;
    }
    if (fstat((int)file.native, &st) != 0) {
        return -1;
    }
    if (st.st_size < 0) {
        return -1;
    }
    *size_out = (uint64_t)st.st_size;
    return 0;
}

int sd_io_close(sd_file *file) {
    if (!file || file->native == SD_FILE_INVALID) {
        return 0;
    }
    if (close((int)file->native) != 0) {
        return -1;
    }
    file->native = SD_FILE_INVALID;
    return 0;
}
