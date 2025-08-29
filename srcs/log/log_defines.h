#ifndef LOG_DEFINES_H
#define LOG_DEFINES_H

typedef enum
{
    false,
    true
} bool;

typedef enum
{
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_BOOT,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
} log_level;

typedef struct
{
    log_level LOG_LEVEL;
    char* LOG_FILE_PATH;
    bool LOG_ERASE;
} log_config;

#endif
