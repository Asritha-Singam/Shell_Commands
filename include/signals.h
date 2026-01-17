// #llm generated code begin
#ifndef SIGNALS_H
#define SIGNALS_H

#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <sys/types.h>

/* Foreground process group ID (set by execution.c while a foreground job runs).
   Signals handlers use this to forward terminal signals to the correct group. */
extern pid_t fg_pgid;

/* Install SIGINT and SIGTSTP handlers (call once at shell startup). */
void install_signal_handlers(void);

/* Handlers (also available if you want to call them manually) */
void sigint_handler(int signo);
void sigtstp_handler(int signo);

#endif // SIGNALS_H
//#llm generated code ends
