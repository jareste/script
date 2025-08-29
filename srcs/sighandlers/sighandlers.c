#define _GNU_SOURCE
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <stdlib.h>

#ifndef TIOCGPTN
# include <linux/tty.h>
#endif

/* Locals for signal handling. */
static pid_t m_child = -1;
static int m_sfd = -1;

static struct termios m_tty_saved;
static int m_saved_ok = 0;

void sigh_tty_restore(void)
{
    if (!m_saved_ok)
        return;

    (void)ioctl(STDIN_FILENO, TCSETS, &m_tty_saved);
    m_saved_ok = 0;
}

#ifdef NOECHO
static int m_tty_raw(void)
{
    struct termios t;

    if (ioctl(STDIN_FILENO, TCGETS, &t) == -1)
        return -1;

    m_tty_saved = t;
    m_saved_ok = 1;

    /* Keep canonical mode for line buffering, disable echo and signals */
    // t.c_iflag &= ~(IXON | ICRNL); /* ICRNL -> converts '\r' to '\n' */
    t.c_lflag &= ~( ISIG | IEXTEN); /* keep echo enabled as i need it. */
    /* if we disable canonical it would pop stdin on each char. */
    /* canon -> processes lines instead of chars */
    // t.c_lflag |= ICANON;  /* Explicitly enable canonical mode */
    /* Output post processing '\n' -> '\r\n' */
    // t.c_oflag &= ~(OPOST);

    if (ioctl(STDIN_FILENO, TCSETS, &t) == -1)
        return -1;
    return 0;
}
#else
static int m_tty_raw(void)
{
    struct termios t;

    if (ioctl(STDIN_FILENO, TCGETS, &t) == -1)
        return -1;

    m_tty_saved = t;
    m_saved_ok = 1;

    /* Keep canonical mode for line buffering, disable signals only */
    t.c_iflag &= ~(IXON | ICRNL);
    t.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);  /* Keep ICANON and ECHO enabled */
    // t.c_lflag |= ICANON;  /* Explicitly enable canonical mode */
    t.c_oflag &= ~(OPOST);

    if (ioctl(STDIN_FILENO, TCSETS, &t) == -1)
        return -1;
    return 0;
}
#endif

static void m_sync_winsize(void)
{
    struct winsize ws;

    if (m_sfd < 0) return;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1) return;

    (void)ioctl(m_sfd, TIOCSWINSZ, &ws);
}

static void m_forward_signal(int sig)
{
    if (m_child > 0)
        kill(m_child, sig);
}

static void m_on_sigwinch(int sig)
{
    (void)sig;
    m_sync_winsize();
}

/* No need to do anything. We just need to avoid the default handler.
 * main loop already waits for child process termination.
 */
static void m_on_sigchld(int sig)
{
    (void)sig;
}

/*
 * SIGTERM: restaurar TTY y enviar señal al hijo
 */
static void m_on_exit_signal(int sig)
{
    sigh_tty_restore();
    m_forward_signal(sig);
    /* No need to kill child directly. PTY and waitpid on loop would handle it properly. */
    _exit(128 + sig);
}


void sigh_init_signals()
{
    struct sigaction sa = {0};

    /* If something goes wrong, program would still work but with 'weird'
     * behaviour. Therefore, as it would be safe, let it be.
     */
    m_tty_raw();

    // atexit(sigh_tty_restore); /* ilegal func, but its useful */

    m_sync_winsize();

    /* SIGWINCH -> update winsize on PTY */
    sa.sa_handler = m_on_sigwinch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, NULL);

    /* SIGCHILD. not signignore but do nothing. */
    sa.sa_handler = m_on_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    /* Signals to forward and also protect our TTY */
    sa.sa_handler = m_on_exit_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGHUP,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
}

void sigh_set_slavefd(int childfd)
{
    m_sfd = childfd;
}

void sigh_set_childfd(int childfd)
{
    m_child = childfd;
}
