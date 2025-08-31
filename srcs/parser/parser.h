#ifndef PARSER_H
#define PARSER_H

#include <unistd.h>
#include <fcntl.h>

#define OPT_NONE 0x0000
#define OPT_INFILE 0x0001
#define OPT_OUTFILE 0x0002
#define OPT_INOUTFILE 0x0004
#define OPT_TIMING 0x0008
#define OPT_timing 0x0010
#define OPT_mlog_format 0x0020
#define OPT_append 0x0040
#define OPT_force 0x0080
#define OPT_ereturnchild 0x0100
#define OPT_fflush 0x0200
#define OPT_ECHO 0x0400
#define OPT_outlimit 0x0800
#define OPT_quiet 0x1000
#define OPT_help 0x2000
#define OPT_version 0x4000

typedef enum
{
    ECHO_AUTO,
    ECHO_ALWAYS,
    ECHO_NEVER
} when_echo;

typedef enum
{
    LOG_ADVANCED = 1,
    LOG_CLASSIC = 2
} log_format;

typedef struct
{
    int options;
    char* command;
    when_echo echo;
    log_format log;
    char* file;
    char* infile;
    char* outfile;
    char* logtime; /* useless? */
    ssize_t outsize;
} parser_t;

int parse(char** argv, parser_t* cfg);

#endif /* PARSER_H */
