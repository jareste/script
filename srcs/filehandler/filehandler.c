#include "filehandler.h"
#include <fcntl.h>

#define FLUSH_LINE(fd, total_written) do { \
    if (head != 0) { \
        ssize_t _w = write(fd, _buf, head); \
        if (_w > 0) *(total_written) += _w; \
        head = 0; \
    } \
} while(0)

#define POP_UTF8() do { \
    if (head != 0) { \
        size_t _i = head - 1; \
        while (_i > 0 && (_buf[_i] & 0xC0) == 0x80) _i--; \
        head = _i; \
    } \
} while(0)

ssize_t fh_write(int fd, const void *buf, size_t count)
{
    /* Usually we should always receive only one byte at a time.
     * That's bc no canonical. but just in case :) 
     */
    static unsigned char _buf[4096];
    static size_t head = 0;
    static int cr_pending = 0;
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

        if (cr_pending && c != '\n')
        {
            head = 0;
            cr_pending = 0;
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
            cr_pending = 1;
            continue;
        }

        if (c == '\n')
        {
            cr_pending = 0;
            FLUSH_LINE(fd, &total_written);
            w = write(fd, "\n", 1);
            if (w > 0)
                total_written += w;
            continue;
        }

        if (head < sizeof(_buf))
        {
            _buf[head++] = c;
        }
        else
        {
            /* full buffer, empty it. */
            FLUSH_LINE(fd, &total_written);
            _buf[head++] = c;
        }
    }

    return total_written;
}

ssize_t fh_flush(int fd)
{
    return fh_write(fd, (const void*)"", 0);
}
