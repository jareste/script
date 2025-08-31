#include "filehandler.h"
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <ft_printf.h>
#include "../log/log.h"
#include "../parser/parser.h"
#include <libft.h>

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

static int is_default_typescript(const char *path)
{
    return path && ft_strncmp(path, "typescript", 11) == 0;
}

static int open_one(const char *path, int base_flags, int force, int check_hardlink)
{
    int flags = base_flags;
    struct stat st;
    int fd;

    if (!force)
        flags |= O_NOFOLLOW;

    fd = open(path, flags, 0644);
    if (fd == -1)
    {
        if (!force && errno == ELOOP)
        {
            log_msg(LOG_LEVEL_DEBUG, "open() failed due to symlink (O_NOFOLLOW): %s\n", path);
            if (ft_strncmp(path, "typescript", 11) == 0)
                return -2;
            return -3;
        }
        return -1;
    }

    if (!force && check_hardlink && is_default_typescript(path))
    {
        if (fstat(fd, &st) == -1)
        {
            close(fd);
            return -1;
        }
        if (st.st_nlink > 1)
        {
            /* “refusing to log to hard-linked default output (use --force)” */
            close(fd);            
            return -2;
        }
    }
    return fd;
}

int fh_open_files(open_fds* fds, parser_t* cfg)
{
    char* in = cfg->infile;
    char* out = cfg->outfile;
    char* both = cfg->file;
    char* timefile = cfg->logtime;
    bool force = cfg->options & OPT_force;
    int base_flags = O_CREAT | O_RDWR | (cfg->options & OPT_append ? 0 : O_TRUNC);
    int tfd_flags;

    fds->in_fd = fds->out_fd = fds->both_fd = -1;
    fds->in_ctx.head = fds->in_ctx.cr_pending = 0;
    fds->out_ctx.head = fds->out_ctx.cr_pending = 0;
    fds->both_ctx.head = fds->both_ctx.cr_pending = 0;

    if (in)
    {
        fds->in_fd = open_one(in, base_flags, force, /*check_hardlink=*/0);
        if (fds->in_fd == -1) goto fail;
    }
    if (out)
    {
        fds->out_fd = open_one(out, base_flags, force, /*check_hardlink=*/1);
        if (fds->out_fd < 0) goto fail;
    }
    if (both)
    {
        fds->both_fd = open_one(both, base_flags, force, /*check_hardlink=*/0);
        if (fds->both_fd == -1) goto fail;
    }

    if ((cfg->options & OPT_timing) || (cfg->logtime && (cfg->options & OPT_TIMING)))
    {
        if (timefile)
        {
            tfd_flags = O_CREAT | O_RDWR | O_TRUNC;
            if (!force) tfd_flags |= O_NOFOLLOW;

            m_time_fd = open(timefile, tfd_flags, 0644);
            if (m_time_fd == -1)
            {
                if (!force && errno == ELOOP)
                    return -3;
                goto fail;
            }
        }
        else
        {
            m_time_fd = STDOUT_FILENO;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &m_last_entry_time);

    return 0;

fail:
    if (fds->in_fd   != -1) close(fds->in_fd);
    if (fds->out_fd  != -1) close(fds->out_fd);
    if (fds->both_fd != -1) close(fds->both_fd);
    if (fds->out_fd == -2) return -2;

    if (fds->in_fd == -3) return -3;
    if (fds->out_fd == -3) return -3;
    if (fds->both_fd == -3) return -3;

    return -1;
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
