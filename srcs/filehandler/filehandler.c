#include "filehandler.h"
#include <fcntl.h>

#define FLUSH_LINE(fd, total_written) do { \
    if (ctx->head != 0) { \
        ssize_t _w = write(fd, ctx->buf, ctx->head); \
        if (_w > 0) *(total_written) += _w; \
        ctx->head = 0; \
    } \
} while(0)

#define POP_UTF8() do { \
    if (ctx->head != 0) { \
        size_t _i = ctx->head - 1; \
        while (_i > 0 && (ctx->buf[_i] & 0xC0) == 0x80) _i--; \
        ctx->head = _i; \
    } \
} while(0)

int fh_open_files(open_fds* fds, char* in, char* out, char* both, int erase)
{
    int flags;

    fds->in_fd = -1;
    fds->out_fd = -1;
    fds->both_fd = -1;

    if (erase)
        flags = O_CREAT | O_RDWR | O_TRUNC;
    else
        flags = O_CREAT | O_RDWR | O_APPEND;

    if (in)
    {
        fds->in_fd = open(in, flags, 0644);
        if (fds->in_fd == -1)
            return -1;
    }

    if (out)
    {
        fds->out_fd = open(out, flags, 0644);
        if (fds->out_fd == -1)
            return -1;
    }

    if (both)
    {
        fds->both_fd = open(both, flags, 0644);
        if (fds->both_fd == -1)
            return -1;
    }

    return 0;
}

ssize_t fh_write(fh_ctx* ctx, int fd, const void *buf, size_t count)
{
    /* Usually we should always receive only one byte at a time.
     * That's bc no canonical. but just in case :) 
     */
    unsigned char c;
    const unsigned char *p = (const unsigned char *)buf;
    size_t i;
    ssize_t total_written = 0;
    ssize_t w;

    if (fd == -1 || (!buf && count))
        return -1;

    for (i = 0; i < count; i++)
    {
        c = p[i];

        if (ctx->cr_pending && c != '\n')
        {
            ctx->head = 0;
            ctx->cr_pending = 0;
        }

        if (c == '\b' || c == 0x7F)
        {
            POP_UTF8();
            continue;
        }

        if (c == '\a')
        {
            continue;
        }

        if (c == '\r')
        {
            ctx->cr_pending = 1;
            continue;
        }

        if (c == '\n')
        {
            ctx->cr_pending = 0;
            FLUSH_LINE(fd, &total_written);
            w = write(fd, "\n", 1);
            if (w > 0)
                total_written += w;
            continue;
        }

        if (ctx->head < sizeof(ctx->buf))
        {
            ctx->buf[ctx->head++] = c;
        }
        else
        {
            /* full buffer, empty it. */
            FLUSH_LINE(fd, &total_written);
            ctx->buf[ctx->head++] = c;
        }
    }

    return total_written;
}

ssize_t fh_flush(fh_ctx* ctx, int fd)
{
    return fh_write(ctx, fd, (const void*)"", 0);
}
