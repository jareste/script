#include "filehandler.h"
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <ft_printf.h>
#include "../log/log.h"

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

static struct timespec m_last_entry_time;
static int m_time_fd = -1;

int fh_open_files(open_fds* fds, char* in, char* out, char* both, char* timefile, int time_on, int erase)
{
    int flags;

    fds->in_fd = -1;
    fds->out_fd = -1;
    fds->both_fd = -1;

    fds->in_ctx.head = 0;
    fds->in_ctx.cr_pending = 0;
    fds->out_ctx.head = 0;
    fds->out_ctx.cr_pending = 0;
    fds->both_ctx.head = 0;
    fds->both_ctx.cr_pending = 0;

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

    if (timefile && time_on)
    {
        m_time_fd = open(timefile, O_CREAT | O_RDWR | O_TRUNC, 0644);
        if (m_time_fd == -1)
            return -1;
    }
    else if (time_on)
    {
        m_time_fd = STDOUT_FILENO;
        clock_gettime(CLOCK_MONOTONIC, &m_last_entry_time);
    }

    return 0;
}

ssize_t fh_write(fh_ctx* ctx, int fd, const void *buf, size_t count, int flush)
{
    /* Usually we should always receive only one byte at a time.
     * That's bc no canonical. but just in case :) 
     */
    unsigned char c;
    const unsigned char *p = (const unsigned char *)buf;
    size_t i;
    ssize_t total_written = 0;
    ssize_t w;
    struct timespec new_time;
    double time_dif_in_s;

    if (fd == -1 || (!buf && count))
        return -1;

    if (flush)
        return write(fd, buf, count);

    for (i = 0; i < count; i++)
    {
        c = p[i];

        if (c == '\0')
            continue;

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
            total_written += w;
            continue;
        }

        if (c < 32 && c != '\t')
            continue;

        if (ctx->head < sizeof(ctx->buf))
        {
            ctx->buf[ctx->head++] = c;
            total_written += 1;
        }
        else
        {
            /* full buffer, empty it. */
            FLUSH_LINE(fd, &total_written);
            ctx->buf[ctx->head++] = c;
        }
    }

    /* Flag 'T' and 't' */
    if (m_time_fd != -1)
    {
        clock_gettime(CLOCK_MONOTONIC, &new_time);
        time_dif_in_s = (new_time.tv_sec - m_last_entry_time.tv_sec)
                    + (new_time.tv_nsec - m_last_entry_time.tv_nsec) / 1e9;
        ft_dprintf(m_time_fd, "%f", time_dif_in_s);
        ft_dprintf(m_time_fd, " %d\n", (int)count);
        log_msg(LOG_LEVEL_DEBUG, "%f %d\n", time_dif_in_s, count);
        clock_gettime(CLOCK_MONOTONIC, &m_last_entry_time);
    }

    return total_written;
}

ssize_t fh_flush(fh_ctx* ctx, int fd)
{
    ssize_t total_written = 0;
    
    if (fd == -1 || !ctx)
        return -1;
        
    FLUSH_LINE(fd, &total_written);
    
    return total_written;
}
