#include "parser.h"
#include <libft.h>
#include <ft_printf.h>

#include <unistd.h>
#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#define LONGEQ(lit, name, namelen) (namelen==ft_strlen(lit) && ft_strncmp(name,lit,namelen)==0)

static int m_is_longopt(const char *s)
{
    return (s && s[0]=='-' && s[1]=='-' && s[2]!=0);
}

static int m_is_shortopt(const char *s)
{
    return (s && s[0]=='-' && s[1]!=0 && s[1]!='-');
}

static int m_streq(const char *a, const char *b)
{
    return ft_strncmp(a,b,(ft_strlen(a)>ft_strlen(b)?ft_strlen(a):ft_strlen(b)))==0;
}

static int longopt_take_value_inline(const char *arg, const char **val_out)
{
    const char *eq = ft_strchr(arg, '=');
    if (eq && *(eq+1))
    {
        *val_out = (eq+1);
        return 1;
    }
    *val_out = NULL;
    return 0;
}

static const char* shortopt_attached_value(const char *arg_rest)
{
    if (arg_rest && *arg_rest)
        return arg_rest;
    return NULL;
}

static void perr(const char *msg)
{
    if (!msg)
        return;
    (void)write(2, msg, ft_strlen(msg));
}

int parse(char **argv, parser_t *cfg)
{
    char* arg;
    char opt;
    const char* name;
    const char* v;
    const char* p;
    const char* attached;
    const char* val;
    int i = 0;
    int has_inline;
    size_t namelen;

    cfg->options = 0;
    cfg->command = NULL;
    cfg->echo = ECHO_AUTO;
    cfg->log = LOG_CLASSIC;
    cfg->file = NULL;
    cfg->infile = NULL;
    cfg->outfile = NULL;
    cfg->logtime = NULL;
    cfg->outsize = 0;

    if (argv && argv[0])
        i = 1;

    for (; argv && argv[i]; i++)
    {
        arg = argv[i];

        if (m_streq(arg, "--"))
        {
            i++;
            break;
        }
        else if (m_is_longopt(arg))
        {
            /* Forma larga: --name o --name=value */
            name = arg + 2;
            v = NULL;
            has_inline = longopt_take_value_inline(name, &v);
            namelen = has_inline ? (size_t)(v - name - 1) : ft_strlen(name);

            if (LONGEQ("log-in", name, namelen))
            { /* --log-in <file> */
                if (!has_inline) { i++; if (!argv[i]) { perr("missing value for --log-in\n"); return -1; } v = argv[i]; }
                cfg->infile = (char*)v;
                cfg->options |= OPT_INFILE;
            }
            else if (LONGEQ("log-out", name, namelen))
            { /* --log-out <file> */
                if (!has_inline) { i++; if (!argv[i]) { perr("missing value for --log-out\n"); return -1; } v = argv[i]; }
                cfg->outfile = (char*)v;
                cfg->options |= OPT_OUTFILE;
            }
            else if (LONGEQ("log-io", name, namelen))
            { /* --log-io <file> */
                if (!has_inline) { i++; if (!argv[i]) { perr("missing value for --log-io\n"); return -1; } v = argv[i]; }
                cfg->file = (char*)v;
                cfg->options |= OPT_INOUTFILE;
            }
            else if (LONGEQ("log-timing", name, namelen))
            { /* --log-timing <file> */
                if (!has_inline) { i++; if (!argv[i]) { perr("missing value for --log-timing\n"); return -1; } v = argv[i]; }
                cfg->logtime = (char*)v;
                cfg->options |= OPT_TIMING;
            }
            else if (LONGEQ("timing", name, namelen))
            { /* --timing[=<file>] */ /* what's the difference??? */
                if (!has_inline) {
                    if (argv[i+1] && argv[i+1][0] != '-') { i++; v = argv[i]; }
                    else v = "stderr";
                }
                cfg->logtime = (char*)v;
                cfg->options |= OPT_timing;
            }
            else if (LONGEQ("logging-format", name, namelen))
            { /* --logging-format <name> */
                if (!has_inline) { i++; if (!argv[i]) { perr("missing value for --logging-format\n"); return -1; } v = argv[i]; }
                if (m_streq(v,"classic"))
                    cfg->log = LOG_CLASSIC;
                else if (m_streq(v,"advanced"))
                    cfg->log = LOG_ADVANCED;
                else { perr("invalid --logging-format value\n"); return -1; }
            }
            else if (LONGEQ("append", name, namelen)) { cfg->options |= OPT_append; }
            else if (LONGEQ("command", name, namelen))
            { /* --command <command> */
                if (!has_inline) { i++; if (!argv[i]) { perr("missing value for --command\n"); return -1; } v = argv[i]; }
                cfg->command = (char*)v;
            }
            else if (LONGEQ("return", name, namelen)) { cfg->options |= OPT_ereturnchild; }
            else if (LONGEQ("flush", name, namelen))  { cfg->options |= OPT_fflush; }
            else if (LONGEQ("force", name, namelen))  { cfg->options |= OPT_force; }
            else if (LONGEQ("echo", name, namelen))
            { /* --echo <when> */
                if (!has_inline) { i++; if (!argv[i]) { perr("missing value for --echo\n"); return -1; } v = argv[i]; }
                if (m_streq(v,"auto")) cfg->echo = ECHO_AUTO;
                else if (m_streq(v,"always")) cfg->echo = ECHO_ALWAYS;
                else if (m_streq(v,"never")) cfg->echo = ECHO_NEVER;
                else { perr("invalid --echo value\n"); return -1; }
            }
            else if (LONGEQ("output-limit", name, namelen))
            { /* --output-limit <size> */
                if (!has_inline) { i++; if (!argv[i]) { perr("missing value for --output-limit\n"); return -1; } v = argv[i]; }
                cfg->outsize = (ssize_t)ft_atossize(v);
            }
            else if (LONGEQ("quiet", name, namelen))   { cfg->options |= OPT_quiet; }
            else if (LONGEQ("help", name, namelen))    { return 1; }
            else if (LONGEQ("version", name, namelen)) { return 2; }
            else
            {
                ft_dprintf(2, "ft_script: unrecognized option '%s'\n", name);
                return -1;
            }
            continue;
        }
        else if (m_is_shortopt(arg))
        {
            p = arg + 1;
            while (*p)
            {
                opt = *p++;

                if (opt=='I' || opt=='O' || opt=='B' || opt=='T' || opt=='t' || opt=='m' || opt=='c' || opt=='E' || opt=='o')
                {
                    /* opciones que pueden/ deben tomar valor */
                    attached = shortopt_attached_value(p);
                    val = NULL;

                    switch (opt)
                    {
                    case 't':
                        if (attached) { val = attached; p += ft_strlen(attached); }
                        else
                        {
                            if (argv[i+1] && argv[i+1][0] != '-') { i++; val = argv[i]; }
                            else val = NULL;
                        }
                        cfg->logtime = (char*)val;
                        cfg->options |= OPT_timing;
                        break;

                    case 'I':
                        if (attached) { val = attached; p += ft_strlen(attached); }
                        else { i++; if (!argv[i]) { perr("missing value for -I\n"); return -1; } val = argv[i]; }
                        cfg->infile = (char*)val;
                        cfg->options |= OPT_INFILE;
                        break;


                    case 'O':
                        if (attached) { val = attached; p += ft_strlen(attached); }
                        else { i++; if (!argv[i]) { perr("missing value for -O\n"); return -1; } val = argv[i]; }
                        cfg->outfile = (char*)val;
                        cfg->options |= OPT_OUTFILE;
                        break;

                    case 'B':
                        if (attached) { val = attached; p += ft_strlen(attached); }
                        else { i++; if (!argv[i]) { perr("missing value for -B\n"); return -1; } val = argv[i]; }
                        cfg->file = (char*)val;
                        cfg->options |= OPT_INOUTFILE;
                        break;

                    case 'T':
                        if (attached) { val = attached; p += ft_strlen(attached); }
                        else { i++; if (!argv[i]) { perr("missing value for -T\n"); return -1; } val = argv[i]; }
                        cfg->logtime = (char*)val;
                        cfg->options |= OPT_TIMING;
                        break;

                    case 'm':
                        if (attached) { val = attached; p += ft_strlen(attached); }
                        else { i++; if (!argv[i]) { perr("missing value for -m\n"); return -1; } val = argv[i]; }
                        if (!m_streq(val,"classic") && !m_streq(val,"advanced")) { perr("invalid -m value\n"); return -1; }
                        if (m_streq(val,"classic")) cfg->log = LOG_CLASSIC;
                        else if (m_streq(val,"advanced")) cfg->log = LOG_ADVANCED;
                        break;

                    case 'c':
                        if (attached) { val = attached; p += ft_strlen(attached); }
                        else { i++; if (!argv[i]) { perr("missing value for -c\n"); return -1; } val = argv[i]; }
                        cfg->command = (char*)val;
                        break;

                    case 'E':
                        if (attached) { val = attached; p += ft_strlen(attached); }
                        else { i++; if (!argv[i]) { perr("missing value for -E\n"); return -1; } val = argv[i]; }
                        if (m_streq(val,"auto")) cfg->echo = ECHO_AUTO;
                        else if (m_streq(val,"always")) cfg->echo = ECHO_ALWAYS;
                        else if (m_streq(val,"never")) cfg->echo = ECHO_NEVER;
                        else { perr("invalid -E value\n"); return -1; }
                        break;
                    case 'o':
                        if (attached) { val = attached; p += ft_strlen(attached); }
                        else { i++; if (!argv[i]) { perr("missing value for -o\n"); return -1; } val = argv[i]; }
                        cfg->outsize = (ssize_t)ft_atossize(val);
                        break;

                    default:
                    {
                        ft_dprintf(2, "script: invalid option -- '%c'\n", opt);
                        return -1;
                    }
                    }
                }
                else
                {
                    if (opt=='a') cfg->options |= OPT_append;
                    else if (opt=='e') cfg->options |= OPT_ereturnchild;
                    else if (opt=='f') cfg->options |= OPT_fflush;
                    else if (opt=='q') cfg->options |= OPT_quiet;
                    else if (opt=='h') return 1;        /* help */
                    else if (opt=='V') return 2;        /* version */
                    else
                    {
                        ft_dprintf(2, "ft_script: invalid option -- '%c'\n", opt);
                        return -1;
                    }
                }
            }
            continue;
        }
        else
        {
            cfg->file = arg;
            cfg->options |= OPT_OUTFILE;
            continue;
        }

        break;
    }

    if (!cfg->file && !cfg->infile && !cfg->outfile)
        cfg->outfile = "typescript";

    return 0;
}
