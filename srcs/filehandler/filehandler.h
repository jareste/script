#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <unistd.h>

ssize_t fh_flush(int fd);
ssize_t fh_write(int fd, const void *buf, size_t count);

#endif /* FILEHANDLER_H */