#ifndef LOG_H
#define LOG_H

#include "log_defines.h"

#ifdef DEBUG
#include <stdio.h>
#include <stdbool.h>

#define LOG_FILE "log.txt"

int log_init();

void log_close(void);

void log_msg(log_level level, const char *fmt, ...);
#else /* DEBUG */
    #define log_init() ((void)0)
    #define log_close() ((void)0)
    #define log_msg(level, fmt, ...) ((void)0)
#endif /* DEBUG */

#endif /* LOG_H */
