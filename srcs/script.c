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

// Linux-specific ioctl numbers
#ifndef TIOCGPTN
# include <linux/tty.h>
#endif

#include "log/log.h" /* illegal calls. debugging purpouse :) */
#include <libft.h>
#include <ft_printf.h>

/* allowed https://x64.syscall.sh */

static int pty_open_master(int *mfd, int *sfd)
{
    int master = open("/dev/ptmx", O_RDWR | O_NOCTTY);
    if (master < 0) return -1;

    /* unlock slave (0 = unlock) */
    int unlock = 0;
    if (ioctl(master, TIOCSPTLCK, &unlock) == -1)
    {
        close(master);
        return -1;
    }
    /* get slave PTY master */
    int ptn;
    if (ioctl(master, TIOCGPTN, &ptn) == -1)
    {
        close(master);
        return -1;
    }
    char slave_path[64];
    snprintf(slave_path, sizeof(slave_path), "/dev/pts/%d", ptn); /* no syscall */
    int slave = open(slave_path, O_RDWR | O_NOCTTY);
    if (slave < 0)
    {
        close(master);
        return -1;
    }

    *mfd = master;
    *sfd = slave;
    return 0;
}

static int copy_loop(int mfd, int types_fd, pid_t child)
{
    char buf[4096];
    int stdin_fd = STDIN_FILENO;
    int stdout_fd = STDOUT_FILENO;
    int status;
    int max_fd;
    int ret;
    fd_set rfds;
    pid_t r;
    ssize_t n;
    ssize_t w;
    ssize_t w2;

    while (true)
    {
        r = waitpid(child, &status, WNOHANG);
        
        FD_ZERO(&rfds);
        FD_SET(stdin_fd, &rfds);
        FD_SET(mfd, &rfds);
        max_fd = (stdin_fd > mfd ? stdin_fd : mfd) + 1;

        ret = select(max_fd, &rfds, NULL, NULL, NULL);
        if (ret < 0)
        {
            if (errno == EINTR) continue;
            return -1;
        }
        /* stdin -> master */
        if (FD_ISSET(stdin_fd, &rfds))
        {
            n = read(stdin_fd, buf, sizeof(buf));
            if (n <= 0)
            {
                /* EOF on stdin -> shutdown write side to child */
                // shutdown(mfd, SHUT_WR); /* not all kernels support on pty; ignore errors */
                break;
            }
            else
            {
                w = write(mfd, buf, n);
                (void)w;
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
            w = write(stdout_fd, buf, n);
            (void)w;
            // (void)stdout_fd;
            /* write to typescript */
            w2 = write(types_fd, buf, n);
            (void)w2;
        }

        if (r == child)
        {
            /* child exited; keep looping to drain master until read() <= 0 */
            /* no action here */
            ;
        }
    }
    return 0;
}

char* ft_getenv(char** envp, const char* var)
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

char* get_basename(const char* path)
{
    char* last_slash;

    if (!path)
        return NULL;

    last_slash = ft_strrchr(path, '/');
    if (last_slash)
        return last_slash + 1;

    return (char*)path;
}

int main(int argc, char **argv, char **envp)
{
    const char *outfile = "typescript";
    int quiet = 0;
    const char *cmd = NULL;
    time_t t;
    pid_t pid;
    int status;
    int file_fd;
    int mfd;
    int sfd;
    char* sh_abs;
    char* sh;
    char* args[4];

    /* TODO parse */
    (void)argc; (void)argv;

    log_init();

    if (pty_open_master(&mfd, &sfd) == -1)
    {
#ifdef DEBUG
        perror("pty_open_master");
#else
        ft_dprintf(2, "Open pty master failed\n");
#endif
        return 1;
    }

    file_fd = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (file_fd < 0)
    {
#ifdef DEBUG
        perror("open typescript");
#else
        ft_dprintf(2, "Open '%s' failed\n", outfile);
#endif
        return 1;
    }

    if (!quiet)
    {
        t = time(NULL);
        ft_dprintf(STDOUT_FILENO, "Script started on %s", ctime(&t)); // nosys
        ft_dprintf(file_fd, "Script started on %s", ctime(&t)); // nosys
    }

    pid = fork();
    if (pid < 0)
    {
#ifdef DEBUG
        perror("fork");
#else
        ft_dprintf(2, "Fork failed\n");
#endif
        return 1;
    }

    if (pid == 0)
    {
        /* Child/slave */
        /* Create new session for slave */
        if (setsid() == -1)
        {
            log_msg(LOG_LEVEL_ERROR, "setsid failed\n");
            _exit(127);
        }

        /* Make slave control tty from now on. */
        if (ioctl(sfd, TIOCSCTTY, 0) == -1)
        {
            log_msg(LOG_LEVEL_ERROR, "ioctl TIOCSCTTY failed\n");
            _exit(127);
        }

        dup2(sfd, STDIN_FILENO);
        dup2(sfd, STDOUT_FILENO);
        dup2(sfd, STDERR_FILENO);
        close(mfd);

        if (cmd)
        {
            sh_abs = ft_getenv(envp, "SHELL");
            sh = get_basename(sh_abs);
            args[0] = sh;
            args[1] = "-c";
            args[2] = (char*)cmd;
            args[3] = NULL;
            execve(sh, args, envp);
        }
        else
        {
            sh_abs = ft_getenv(envp, "SHELL");
            sh = get_basename(sh_abs);
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
    copy_loop(mfd, file_fd, pid);

    waitpid(pid, &status, 0);

    if (!quiet)
    {
        t = time(NULL);
        ft_dprintf(file_fd, "\nScript done on %s", ctime(&t));
        ft_dprintf(STDOUT_FILENO, "\nScript done on %s", ctime(&t));
    }
    close(file_fd);
    close(mfd);
    return 0;
}
