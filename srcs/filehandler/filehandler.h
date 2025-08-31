#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <unistd.h>
#include <sys/ioctl.h>
#include "../parser/parser.h"

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
    char* in_filename;

    int out_fd;
    fh_ctx out_ctx;
    char* out_filename;

    int both_fd;
    fh_ctx both_ctx;
    char* both_filename;
} open_fds;

typedef enum
{
    LOG_INTERN = 'H',
    LOG_IN = 'I',
    LOG_OUT = 'O'
} log_adv;

ssize_t fh_flush(fh_ctx* ctx, int fd);
ssize_t fh_write(log_adv type, fh_ctx* ctx, int fd, const void *buf, size_t count, int flush);
int fh_open_files(open_fds* fds, parser_t* cfg);
void fh_set_adv_logging();
void fh_write_time_header(char* time_str, char* term, char* tty, struct winsize ws, open_fds* fds);
void fh_write_time_finals(int exit_code);

#endif /* FILEHANDLER_H */