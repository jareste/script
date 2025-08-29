#include "log_defines.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

#define LOG_FILE "log.txt"

static FILE *m_log_fp = NULL;
static log_level m_log_threshold = -1;
static log_config m_log_config;

int log_init()
{
    char* options;

    m_log_config.LOG_LEVEL = LOG_LEVEL_DEBUG;
    m_log_config.LOG_FILE_PATH = LOG_FILE;
    m_log_config.LOG_ERASE = true;

    options = m_log_config.LOG_ERASE == true ? "w+" : "a";
    m_log_threshold = 0;
    
    m_log_fp = fopen(m_log_config.LOG_FILE_PATH, options);
    if (!m_log_fp)
    {
        perror("fopen");
        return -1;
    }
    return 0;
}

void log_close(void)
{
    if (m_log_fp)
    {
        fclose(m_log_fp);
        m_log_fp = NULL;
    }
}

void log_msg(log_level level, const char *fmt, ...)
{
    va_list args, args_copy;
    static bool inside_logger = false;
    time_t now = time(NULL);
    struct tm *utc_tm = gmtime(&now);
    char time_str[32];

    if (!m_log_fp) return;

    /* avoid circle reference with time api!! */
    if (inside_logger)
        return;
    
    inside_logger = true;

    va_start(args, fmt);
    va_copy(args_copy, args);

    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S UTC", utc_tm);

    fprintf(m_log_fp, "[%s] ", time_str);
    vfprintf(m_log_fp, fmt, args);
    fflush(m_log_fp);

    if ((level <= m_log_threshold) || (level == LOG_LEVEL_BOOT))
    {
        switch (level)
        {
            case LOG_LEVEL_ERROR:
                fprintf(stderr, "\033[1;31m[%s] ", time_str);
                vfprintf(stderr, fmt, args_copy);
                fprintf(stderr, "\033[0m");
                break;
            case LOG_LEVEL_WARN:
                fprintf(stderr, "\033[1;33m[%s] ", time_str);
                vfprintf(stderr, fmt, args_copy);
                fprintf(stderr, "\033[0m");
                break;
            default:
                fprintf(stdout, "[%s] ", time_str);
                vfprintf(stdout, fmt, args_copy);
                break;
        }
    }

    va_end(args_copy);
    va_end(args);
    inside_logger = false;
}