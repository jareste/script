#ifndef SIGHANDLERS_H
#define SIGHANDLERS_H

void sigh_init_signals(void);
void sigh_set_childfd(int childfd);
void sigh_set_slavefd(int slavefd);
void sigh_tty_restore(void);

#endif /* SIGHANDLERS_H */