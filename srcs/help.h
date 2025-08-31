#ifndef HELP_H
#define HELP_H

#define HELP_MSG    "Usage: ft_script [options] [file]\n\n" \
                    "Make a typescript of a terminal session.\n" \
                    "Options:\n" \
                    " -I, --log-in <file>           log stdin to file\n" \
                    " -O, --log-out <file>          log stdout to file (default)\n" \
                    " -B, --log-io <file>           log stdin and stdout to file\n\n" \
                    " -T, --log-timing <file>       log timing information to file\n" \
                    " -t[<file>], --timing[=<file>] deprecated alias to -T (default file is stderr)\n" \
                    " -m, --logging-format <name>   force to 'classic' or 'advanced' format\n\n" \
                    " -a, --append                  append to the log file\n" \
                    " -c, --command <command>       run command rather than interactive shell\n" \
                    " -e, --return                  return exit code of the child process\n" \
                    " -f, --flush                   run flush after each write\n" \
                    "     --force                   use output file even when it is a link\n" \
                    " -E, --echo <when>             echo input in session (auto, always or never)\n" \
                    " -o, --output-limit <size>     terminate if output files exceed size\n" \
                    " -q, --quiet                   be quiet\n\n" \
                    " -h, --help                    display this help\n" \
                    " -V, --version                 display version\n"

#endif