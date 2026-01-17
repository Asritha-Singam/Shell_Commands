//#llm generated code begins
// execution.c
#define _XOPEN_SOURCE 700 // for WCONTINUED
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "execution.h"
#include "fileredirection.h"
#include "intrinsics.h"
#include "signals.h"

#define SHELL_MAX_ARGS 64

/* job structures (left as you had them) */
Job jobs[MAX_JOBS];
int job_count = 0;
pid_t fg_pgid = -1;

static int next_job_id = 1;

/* check_background_jobs unchanged (keeps original logic) */
int check_background_jobs() {
    int status;
    pid_t pid;
    for (int i = 0; i < job_count; ) {
        pid = waitpid(jobs[i].pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if (pid == jobs[i].pid) {
            printf("%s with pid %d ", jobs[i].command, jobs[i].pid);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                printf("exited normally\n");
                jobs[i].state = 2;
            } else if (WIFSIGNALED(status)) {
                printf("exited abnormally\n");
                jobs[i].state = 2;
            } else if (WIFSTOPPED(status)) {
                printf("stopped\n");
                jobs[i].state = 1;
            } else if (WIFCONTINUED(status)) {
                printf("continued\n");
                jobs[i].state = 0;
            }
            fflush(stdout);
            if (jobs[i].state == 2) {
                free(jobs[i].command);
                for (int j = i; j < job_count - 1; j++) {
                    jobs[j] = jobs[j + 1];
                }
                job_count--;
            } else {
                i++;
            }
        } else {
            i++;
        }
    }
    return 1;
}

/* Helper to set child default signals (so child reacts normally to Ctrl-C / Ctrl-Z) */
static void restore_default_signals_in_child(void) {
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
}

/* execute_commands: mostly your original logic; adjusted only where needed for pgid/tcsetpgrp */
void execute_commands(char **argv, int argc) {
    int start_pos = 0;
    for (int i = 0; i <= argc; i++) {
        if (i == argc || strcmp(argv[i], ";") == 0 || strcmp(argv[i], "&") == 0) {
            char *sub_argv[SHELL_MAX_ARGS];
            int sub_argc = 0;
            for (int j = start_pos; j < i; j++) sub_argv[sub_argc++] = argv[j];
            sub_argv[sub_argc] = NULL;

            if (sub_argc > 0) {
                int intrinsic_type = check(sub_argc, sub_argv);

                if (intrinsic_type == 0) {
                    if(strcmp(sub_argv[0] ,"hop")==0){
                        hop_cmd(sub_argc, sub_argv);
                    }
                    else if(strcmp(sub_argv[0], "fg")==0){
                        fg_cmd(sub_argc,sub_argv);
                    }else if(strcmp(sub_argv[0],"bg")==0){
                        bg_cmd(sub_argc,sub_argv);
                    }
                } else {
                    bool is_background = (i < argc && strcmp(argv[i], "&") == 0);
                    pid_t pid = fork();
                    if (pid < 0) {
                        perror("fork");
                        start_pos = i + 1;
                        continue;
                    }

                    if (pid == 0) {
                        /* ---------------- CHILD ---------------- */
                        /* Put child in its own process group */
                        if (setpgid(0, 0) < 0) {
                            /* non-fatal, but print for debugging */
                            // perror("child setpgid");
                        }

                        /* restore default signal dispositions in child */
                        restore_default_signals_in_child();

                        if (is_background) {
                            int null_fd = open("/dev/null", O_RDONLY);
                            if (null_fd != -1) {
                                dup2(null_fd, STDIN_FILENO);
                                close(null_fd);
                            }
                        }

                        /* Execute command (your existing handler) */
                        int command_type = check(sub_argc, sub_argv);
                        if (handle_command_group(sub_argv, sub_argc, command_type) == -1) {
                            _exit(1);
                        }
                        _exit(0);
                    } else {
                        /* ---------------- PARENT ---------------- */
                        /* Ensure child is in its own pgid (race check) */
                        if (setpgid(pid, pid) < 0 && errno != EINVAL) {
                            /* EINVAL may occur if child already set pgid; ignore race */
                            // perror("parent setpgid");
                        }

                        if (!is_background) {
                            int status;
                            /* Give terminal to child process group (parent does tcsetpgrp) */
                            /* temporarily ignore SIGTTOU while switching (shell already ignores at startup but keep safe) */
                            void (*old_sig_ttou)(int) = signal(SIGTTOU, SIG_IGN);
                            if (tcsetpgrp(STDIN_FILENO, pid) < 0) {
                                /* not fatal in non-interactive cases, but log for debugging */
                                // perror("tcsetpgrp(to child)");
                            }
                            signal(SIGTTOU, old_sig_ttou);

                            fg_pgid = pid;

                            /* Wait for child to exit or stop */
                            waitpid(pid, &status, WUNTRACED);

                            if (WIFSTOPPED(status)) {
                                /* save stopped foreground job in separate array (unchanged semantics) */
                                if (job_count < MAX_JOBS) {
                                    jobs[job_count].pid = pid;
                                    jobs[job_count].pgid = pid;
                                    jobs[job_count].job_id = next_job_id++;
                                    jobs[job_count].command = strdup(sub_argv[0]);
                                    jobs[job_count].state = 1; /* Stopped */
                                    printf("[%d] Stopped %s\n",jobs[job_count].job_id, sub_argv[0]);
                                    fflush(stdout);
                                    job_count++;
                                } else {
                                    fprintf(stderr, "job table full; dropping stopped job\n");
                                }
                            }

                            /* Restore terminal to shell */
                            if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0) {
                                // perror("tcsetpgrp(shell)");
                            }
                            fg_pgid = -1;
                        } else {
                            /* Background job: keep existing jobs[] logic (unchanged semantics) */
                            if (job_count < MAX_JOBS) {
                                jobs[job_count].pid = pid;
                                jobs[job_count].pgid = pid;
                                jobs[job_count].job_id = next_job_id++;
                                jobs[job_count].command = strdup(sub_argv[0]);
                                jobs[job_count].state = 0;
                                printf("[%d] %d\n", jobs[job_count].job_id, pid);
                                fflush(stdout);
                                job_count++;
                            } else {
                                fprintf(stderr, "background job table full\n");
                            }
                        }
                    }
                }
            }
            start_pos = i + 1;
        }
    }
}
//# llm generated code ends