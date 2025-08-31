#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef TIOCGPTN
# include <linux/tty.h>
#endif

#include "log/log.h" /* illegal calls. debugging purpouse :) */
#include "sighandlers/sighandlers.h"
#include "filehandler/filehandler.h"
#include "parser/parser.h"
#include "help.h"
#include <libft.h>
#include <ft_printf.h>

/* allowed https://x64.syscall.sh */

static int m_pty_open_master(int *mfd, int *sfd, char slave_path[64])
{
    int master;
    int unlock = 0;
    int ptn;
    int slave;
    char itoa_buf[20] = { 0 };

    master = open("/dev/ptmx", O_RDWR | O_NOCTTY);
    if (master < 0) return -1;

    /* unlock slave (0 = unlock) */
    if (ioctl(master, TIOCSPTLCK, &unlock) == -1)
    {
        close(master);
        return -1;
    }
    /* get slave PTY master */
    if (ioctl(master, TIOCGPTN, &ptn) == -1)
    {
        close(master);
        return -1;
    }

    memset(itoa_buf, 0, sizeof(itoa_buf));
    ft_itoa_nc(ptn, itoa_buf);

    strncpy(slave_path, "/dev/pts/", 64);
    strncat(slave_path, itoa_buf, 64 - strlen(slave_path) - 1);
    log_msg(LOG_LEVEL_DEBUG, "slave path: %s\n", slave_path);
    slave = open(slave_path, O_RDWR | O_NOCTTY);
    if (slave < 0)
    {
        close(master);
        return -1;
    }

    *mfd = master;
    *sfd = slave;
    sigh_set_slavefd(slave);
    return 0;
}

static int m_get_exit_code(pid_t child, int* exit_code, int* signal)
{
    int r;
    int status;

    if (!exit_code || !signal)
        return -1;

    r = waitpid(child, &status, WNOHANG);
    if (r == child)
    {
        if (WIFEXITED(status))
        {
            *exit_code = WEXITSTATUS(status);
            log_msg(LOG_LEVEL_DEBUG, "Child exited with code: %d\n", *exit_code);
            return 1;
        }
        else if (WIFSIGNALED(status))
        {
            *signal = WTERMSIG(status);
            log_msg(LOG_LEVEL_DEBUG, "Child killed by signal: %d\n", *signal);
            return 2;
        }
    }
    return 0;
}

static int m_copy_loop(int mfd, open_fds* fds, pid_t child, bool echo, bool flush, ssize_t out_limit)
{
    char buf[4096];
    int stdin_fd = STDIN_FILENO;
    int stdout_fd = STDOUT_FILENO;
    int max_fd;
    int ret;
    int exit_code = -1;
    int signal = -1;
    fd_set rfds;
    pid_t r;
    ssize_t n;
    time_t t;
    char* time_str;
    ssize_t total_written = 0;

    log_msg(LOG_LEVEL_DEBUG, "starting copy loop with echo [%d], flush [%d], out_limit [%zd]\n", echo, flush, out_limit);
    while (true)
    {
        r = m_get_exit_code(child, &exit_code, &signal);
        if (r == 1 || r == 2)
            break;
        else if (r == -1)
            return -1;

        FD_ZERO(&rfds);
        FD_SET(stdin_fd, &rfds);
        FD_SET(mfd, &rfds);
        max_fd = (stdin_fd > mfd ? stdin_fd : mfd) + 1;

        ret = select(max_fd, &rfds, NULL, NULL, NULL);
        if (ret < 0)
        {
            log_msg(LOG_LEVEL_ERROR, "select() failed: %s\n", strerror(errno));
            if (errno == EINTR) continue;
            return -1;
        }
        /* stdin -> master */
        if (FD_ISSET(stdin_fd, &rfds))
        {
            n = read(stdin_fd, buf, sizeof(buf));
            if (n <= 0)
            { /* EOF on stdin -> shutdown write side to child */
                write(stdout_fd, "exit\r\n", 7);
                write(mfd, "exit\r\n", 7);
                break;
            }
            else
            {
                write(mfd, buf, n);
                write(fds->in_fd, buf, n);
                log_msg(LOG_LEVEL_DEBUG, "Wrote %zd bytes from stdin to master\n", n);
            }
        }
        /* master -> stdout + file */
        if (FD_ISSET(mfd, &rfds))
        {
            n = read(mfd, buf, sizeof(buf));
            if (n <= 0)
            {
                /* either closed or error so go out. */
                break;
            }
            /* echo to screen */
            if (echo)
                write(stdout_fd, buf, n);
            /* write to file */
            total_written += fh_write(&fds->both_ctx, fds->both_fd, buf, n, flush);
            fh_write(&fds->out_ctx, fds->out_fd, buf, n, flush);
        }

        if (total_written >= out_limit && (out_limit != 0))
            break;
    }

    fh_flush(&fds->both_ctx, fds->both_fd); /* I think it's not needed tbh */
    fh_flush(&fds->out_ctx, fds->out_fd);

    t = time(NULL);
    time_str = ctime(&t);
    *strchr(time_str, '\n') = '\0';

    ft_dprintf(fds->both_fd, "\nScript done on %s ", time_str);
    ft_dprintf(fds->out_fd, "\nScript done on %s ", time_str);
    ft_dprintf(fds->in_fd, "\nScript done on %s ", time_str);

    if (total_written >= out_limit && (out_limit != 0))
    { /* max output exceeded */
        ft_dprintf(fds->both_fd, "[<max output size exceeded>]\r\n");
        ft_dprintf(fds->out_fd, "[<max output size exceeded>]\r\n");
        ft_dprintf(fds->in_fd, "[<max output size exceeded>]\r\n");

        kill(SIGTERM, child);
        return 2;
    }
    else
    {
        if (r == 0)
            r = m_get_exit_code(child, &exit_code, &signal);
        switch (r)
        {
            case 1:
                log_msg(LOG_LEVEL_DEBUG, "Child exited!!!!\n");
                ft_dprintf(fds->both_fd, "[COMMAND_EXIT_CODE=\"%d\"]\r\n", exit_code);
                ft_dprintf(fds->out_fd, "[COMMAND_EXIT_CODE=\"%d\"]\r\n", exit_code);
                ft_dprintf(fds->in_fd, "[COMMAND_EXIT_CODE=\"%d\"]\r\n", exit_code);
                break;
            case -1:
                return -1;
            case 2:
                log_msg(LOG_LEVEL_DEBUG, "Child killed by signal: %d\n", signal);
                /* fallthrough */ /* KCH */
            default:
                ft_dprintf(fds->both_fd, "\b\b [COMMAND_EXIT_CODE=\"0\"]\r\n");
                ft_dprintf(fds->out_fd, "\b\b [COMMAND_EXIT_CODE=\"0\"]\r\n");
                ft_dprintf(fds->in_fd, "\b\b [COMMAND_EXIT_CODE=\"0\"]\r\n");
                break;
        }
    }

    return 0;
}

static char* m_ft_getenv(char** envp, const char* var)
{
    size_t len;
    char** env;

    len = ft_strlen(var);
    for (env = envp; *env != 0; env++)
    {
        if (ft_strncmp(*env, var, len) == 0 && (*env)[len] == '=')
        {
            return *env + len + 1;
        }
    }
    return NULL;
}

static char* m_get_basename(const char* path)
{
    char* last_slash;

    if (!path)
        return NULL;

    last_slash = ft_strrchr(path, '/');
    if (last_slash)
        return last_slash + 1;

    return (char*)path;
}

static void m_write_script_header(open_fds* fds, char* tty, char** envp, parser_t* cfg)
{
    struct winsize ws;
    time_t t;
    char *term = m_ft_getenv(envp, "TERM");
    int i;
    int fd;
    char* time_str;

    t = time(NULL);
    time_str = ctime(&t);
    *strchr(time_str, '\n') = '\0';


    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1)
    {
        ws.ws_col = 80;
        ws.ws_row = 24;
    }

    if (!(cfg->options & OPT_quiet))
    {
        ft_dprintf(STDOUT_FILENO, "Script started");
        if ((cfg->options & OPT_OUTFILE) || (cfg->options & OPT_INOUTFILE))
            ft_dprintf(STDOUT_FILENO, ", output log file is '%s'", cfg->file ? cfg->file : cfg->outfile);
        if ((cfg->options & OPT_INFILE) || (cfg->options & OPT_INOUTFILE))
            ft_dprintf(STDOUT_FILENO, ", input log file is '%s'", cfg->file ? cfg->file : cfg->infile);

        ft_dprintf(STDOUT_FILENO, ".\n");
    }

    for (i = 0; i < 3; i++)
    {
        switch (i)
        {
        case 0:
            fd = fds->both_fd;
            break;
        case 1:
            fd = fds->out_fd;
            break;
        case 2:
            fd = fds->in_fd;
            break;
        }

        ft_dprintf(fd, "Script started on %s [TERM=\"%s\" TTY=\"%s\" COLUMNS=\"%d\" LINES=\"%d\"]\n",
                time_str,
                term ? term : "unknown",
                tty ? tty : "unknown", 
                ws.ws_col,
                ws.ws_row);
    }
}

static void cleanup_child(pid_t child)
{
    if (child > 0)
    {
        log_msg(LOG_LEVEL_DEBUG, "Cleaning up child process %d\n", child);
        kill(child, SIGTERM);

        usleep(100000);

        if (kill(child, 0) == 0)
        {
            log_msg(LOG_LEVEL_DEBUG, "Force killing child process %d\n", child);
            kill(child, SIGKILL);
        }

        waitpid(child, NULL, WNOHANG);
        child = 0;
    }
}

static int m_exec_child(char** envp, parser_t* cfg, int sfd, int mfd)
{
    pid_t pid;
    char* sh_abs;
    char* sh;
    char* args[4];

   pid = fork();
    if (pid < 0)
    {
        ft_dprintf(2, "Fork failed\n");
        return -1;
    }

    if (pid == 0)
    {
        sigh_set_childfd(pid);
        /* Child/slave */
        /* Create new session for slave */
        if (setsid() == -1)
            _exit(127);

        /* Make slave control tty from now on. */
        if (ioctl(sfd, TIOCSCTTY, 0) == -1)
            _exit(127);

        dup2(sfd, STDIN_FILENO);
        dup2(sfd, STDOUT_FILENO);
        dup2(sfd, STDERR_FILENO);
        close(mfd);

        if (cfg->command)
        {
            sh_abs = m_ft_getenv(envp, "SHELL");
            sh = m_get_basename(sh_abs);
            args[0] = sh;
            args[1] = "-c";
            args[2] = (char*)cfg->command;
            args[3] = NULL;
            execve(sh_abs, args, envp);
        }
        else
        {
            sh_abs = m_ft_getenv(envp, "SHELL");
            sh = m_get_basename(sh_abs);
            args[0] = sh;
            args[1] = "-i";
            args[2] = NULL;
            log_msg(LOG_LEVEL_DEBUG, "Starting interactive shell: %s\n", sh_abs);
            log_msg(LOG_LEVEL_DEBUG, "Sending args: %s\n", args[0]);
            execve(sh_abs, args, envp);
        }
        _exit(127);
    }

    close(sfd);
    return pid;
}

static int m_parse_error(int ret)
{
    switch (ret)
    {
        case -1:
            ft_dprintf(2, "Unknown error occurred\n");
            return 1;
        case -2:
            ft_dprintf(2, "Symbolic link error occurred\n");
            return 0;
        case -3:
            ft_dprintf(2, "Another type of error occurred\n");
            return 0;
    }
    return 0;
}

int main(int argc, char **argv, char **envp)
{
    open_fds fds;
    int mfd = -1;
    int sfd;
    int ret;
    pid_t child;
    char slave_path[64] = { 0 };;
    parser_t cfg;
    bool echo;
    bool flush;

    (void)argc;

    ret = parse(argv, &cfg);
    if (ret != 0)
        return m_parse_error(ret);

    log_init();

    if (m_pty_open_master(&mfd, &sfd, slave_path) == -1)
        goto error;

    ret = fh_open_files(&fds, &cfg);
    if (ret == -1)
    { /* UNKNOWN error*/
        goto error;
    }
    else if (ret == -2 || ret == -3)
    { /* Simlink related error. */
        if (ret == -2)
            ft_dprintf(2, "ft_script: output file `typescript' is a link\r\n");
        else
            ft_dprintf(2, "ft_script: will not follow symbolic link\r\n");

        ft_dprintf(2, "Use --force if you really want to use it.\r\n");
        ft_dprintf(2, "Program not started.\r\n");
        goto error;
    }

    m_write_script_header(&fds, slave_path, envp, &cfg);

    sigh_init_signals();

    child = m_exec_child(envp, &cfg, sfd, mfd);
    if (child == -1)
        goto error;

    echo = cfg.echo == ECHO_ALWAYS || (cfg.echo == ECHO_AUTO && isatty(STDIN_FILENO));
    flush = cfg.options & OPT_fflush;
    ret = m_copy_loop(mfd, &fds, child, echo, flush, cfg.outsize);

    if (ret == 2)
        cleanup_child(child);

    if (!(cfg.options & OPT_quiet))
    {
        if (ret == 2)
            ft_dprintf(STDOUT_FILENO, "Script terminated, max output files size %u exceeded.\n", (unsigned int)cfg.outsize); /* dont want to create %zu just for this. */

        ft_dprintf(STDOUT_FILENO, "Script done.\r\n");
    }

    close(fds.both_fd);
    close(mfd);
    log_close();
    sigh_tty_restore();
    return 0;

error:
    close(fds.both_fd);
    close(mfd);
    log_close();
    return 1;
}
