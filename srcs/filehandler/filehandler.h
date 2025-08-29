#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <unistd.h>

typedef struct
{
    unsigned char buf[1024];
    size_t head;
    int cr_pending;
} fh_ctx;

typedef struct
{
    int in_fd;
    fh_ctx in_ctx;

    int out_fd;
    fh_ctx out_ctx;

    int both_fd;
    fh_ctx both_ctx;
} open_fds;

ssize_t fh_flush(fh_ctx* ctx, int fd);
ssize_t fh_write(fh_ctx* ctx, int fd, const void *buf, size_t count);
int fh_open_files(open_fds* fds, char* in, char* out, char* both, int erase);

#endif /* FILEHANDLER_H */