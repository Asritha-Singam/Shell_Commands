//#llm generated code begins
#define _POSIX_C_SOURCE 200809L

#include "signals.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

/* fg_pgid is defined in execution.c */
extern pid_t fg_pgid;

/* Forward SIGINT (Ctrl-C) to the foreground process group */
void sigint_handler(int signo) {
    if (fg_pgid > 0) {
        kill(-fg_pgid, SIGINT); /* negative pgid -> send to process group */
    }
    /* Do not terminate the shell here */
}

/* Forward SIGTSTP (Ctrl-Z) to the foreground process group */
void sigtstp_handler(int signo) {
    if (fg_pgid > 0) {
        kill(-fg_pgid, SIGTSTP);
    }
    /* shell keeps running; check_background_jobs() will detect stopped jobs */
}

void install_signal_handlers(void) {
    struct sigaction sa;

    /* SIGINT */
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    /* SIGTSTP */
    sa.sa_handler = sigtstp_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, NULL);
}
//#llm generated code ends