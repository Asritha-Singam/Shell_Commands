// shell.c
//---#llm generated code begins
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>

#include "parse_input.h"
#include "intrinsics.h"
#include "fileredirection.h"
#include "execution.h"
#include "signals.h"

#define SHELL_MAX_INPUT 1024
#define SHELL_MAX_ARGS 64

extern Job jobs[MAX_JOBS];
extern int job_count;


char shell_home_dir[PATH_MAX];

static void show_prompt(void) {
    struct passwd *pw = getpwuid(getuid());
    char hostname[256];
    char current_dir[PATH_MAX];
    char display_path[PATH_MAX];

    if (!getcwd(current_dir, sizeof(current_dir))) {
        perror("getcwd");
        strcpy(current_dir, "?");
    }

    if (strcmp(shell_home_dir, current_dir) == 0) {
        strcpy(display_path, "~");
    } else if (strncmp(shell_home_dir, current_dir, strlen(shell_home_dir)) == 0) {
        snprintf(display_path, sizeof(display_path), "~%s", current_dir + strlen(shell_home_dir));
    } else {
        strcpy(display_path, current_dir);
    }

    if (pw && gethostname(hostname, sizeof(hostname)) == 0) {
        printf("<%s@%s:%s> ", pw->pw_name, hostname, display_path);
    } else {
        printf("<unknown@unknown:?> ");
    }
    fflush(stdout);
}

int main(void) {
    char input[SHELL_MAX_INPUT];
    char *argv[SHELL_MAX_ARGS];

    if (!getcwd(shell_home_dir, sizeof(shell_home_dir))) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    /* If interactive, ensure shell has its own process group and owns the terminal */
    if (isatty(STDIN_FILENO)) {
        pid_t shell_pgid = getpid();
        /* try to put shell in its own process group */
        if (setpgid(shell_pgid, shell_pgid) < 0 && errno != EINVAL && errno != EPERM) {
            perror("setpgid(shell)");
        }
        /* ignore TTY background-stop signals so shell won't get stopped by tcsetpgrp ops */
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);

        /* best-effort: give terminal to shell pgid */
        if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) {
            /* not fatal; may be non-interactive or permission issue */
            // perror("tcsetpgrp(shell)");
        }
    }

    /* Initialize log on shell start */
    log_init();

    /* Make shell ignore Ctrl-C/Z by default and install custom handlers that forward */
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    install_signal_handlers();

    while (1) {
        check_background_jobs();
        show_prompt();

        if (fgets(input, sizeof(input), stdin) == NULL) {
            /* EOF (Ctrl-D) or error */
            printf("\nlogout\n");
            fflush(stdout);

            /* Kill background jobs and stopped-fg jobs */
            for (int i = 0; i < job_count; ++i) kill(-jobs[i].pgid, SIGKILL);
            exit(0);
        }

        /* strip trailing newline if present */
        size_t L = strlen(input);
        if (L && input[L-1] == '\n') input[L-1] = '\0';

        if (input[0] == '\0') continue;

        /* Logging logic */
        bool loggable = true;
        char *temp_input = strdup(input);
        if (temp_input) {
            char *first_token = strtok(temp_input, " \t\n");
            if (first_token && strcmp(first_token, "log") == 0) loggable = false;
            free(temp_input);
        }
        if (loggable) log_add(input);

        int argc = parse_input(input, argv, SHELL_MAX_ARGS);
        if (argc == 0) continue;

        execute_commands(argv, argc);
    }

    return 0;
}
//#llm generated code ends