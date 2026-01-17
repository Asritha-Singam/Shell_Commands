// #llm generated llm code begins
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "fileredirection.h"
#include "intrinsics.h"
#include "execution.h"

#define SHELL_MAX_ARGS 64



// Function to handle input and output redirection
static int handle_redirections(char **argv, int argc) {
    int input_fd = -1;
    int output_fd = -1;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "<") == 0) {
            // Input redirection
            if (i + 1 >= argc) {
                fprintf(stderr, "No such file or directory\n");
                return -1;
            }
            if (input_fd != -1) close(input_fd); // Close previous input file
            input_fd = open(argv[i+1], O_RDONLY);
            if (input_fd < 0) {
                printf("No such file or directory\n");
                return -1;
            }
            argv[i] = NULL; // Null terminate the argument list for execvp
            i++;
        } else if (strcmp(argv[i], ">") == 0) {
            // Output redirection (overwrite)
            if (i + 1 >= argc) {
                fprintf(stderr, "Unable to create file for writing\n");
                return -1;
            }
            if (output_fd != -1) close(output_fd); // Close previous output file
            output_fd = open(argv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output_fd < 0) {
                printf("Unable to create file for writing\n");
                return -1;
            }
            argv[i] = NULL;
            i++;
        } else if (strcmp(argv[i], ">>") == 0) {
            // Output redirection (append)
            if (i + 1 >= argc) {
                fprintf(stderr, "Unable to create file for writing\n");
                return -1;
            }
            if (output_fd != -1) close(output_fd); // Close previous output file
            output_fd = open(argv[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (output_fd < 0) {
                printf("Unable to create file for writing\n");
                return -1;
            }
            argv[i] = NULL;
            i++;
        }
    }

    if (input_fd != -1) {
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }
    if (output_fd != -1) {
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }

    return 0;
}

// Function to execute a single command, including redirections
static int execute_single_command(char **argv, int argc,int command_type) {
    if (handle_redirections(argv, argc) == -1) {
        _exit(1);
    }
    
    if(command_type == 1) { // It's an intrinsic command
        run_intrinsic(argc, argv);
        _exit(0); // Exit after running the intrinsic
    } else { // It's an external command
        execvp(argv[0], argv);
        fprintf(stderr,"Command not found!\n");
    }
    
    _exit(1); // Should only reach here on failure
}


// Function to handle a command group (with pipes)
int handle_command_group(char **argv, int argc, int command_type) {
    if (argc == 0) return 0;
    int num_pipes = 0;
    for(int i = 0; i < argc; i++) {
        if(strcmp(argv[i], "|") == 0) {
            num_pipes++;
        }
    }

    if (num_pipes == 0) {
        // No pipes, just execute a single command
        execute_single_command(argv, argc, command_type);
        return 0;
    }

    //int current_pipe_index = 0;
    int pipefd[2];
    int prev_pipe_read_end = STDIN_FILENO;
    int command_start_index = 0;
    pid_t pgid =0;
    int leader_pid=0;

    for (int i = 0; i <= argc; i++) {
        if (i == argc || strcmp(argv[i], "|") == 0) {
            char *sub_argv[SHELL_MAX_ARGS];
            int sub_argc = 0;
            for(int j = command_start_index; j < i; j++) {
                sub_argv[sub_argc++] = argv[j];
                
            }
            
            sub_argv[sub_argc] = NULL;
            
            if (sub_argc == 0) {
                fprintf(stderr, "Invalid null command in pipe\n");
                return 1;
            }

            if (i != argc) { // Not the last command
                if (pipe(pipefd) < 0) {
                    perror("pipe");
                    return -1;
                }
            }

            int pid = fork();
            if (pid < 0) {
                perror("fork");
                return -1;
            }
            
            if (pid == 0) {
                // Child process
                if (pgid == 0) {
                    setpgid(0, 0);        // leader sets its own pgid
                } else {
                    setpgid(0, pgid);     // join leader's group
                }
                
                if (prev_pipe_read_end != STDIN_FILENO) {
                    dup2(prev_pipe_read_end, STDIN_FILENO);
                    close(prev_pipe_read_end);
                }
                if (i != argc) {
                    close(pipefd[0]); // Close read end in child
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[1]); // Close write end in child
                }
                
                int sub_command_type = check(sub_argc, sub_argv);
                execute_single_command(sub_argv, sub_argc, sub_command_type);
                
                _exit(1); // Should not reach here
            }
            else {
                /* ---------------- PARENT ---------------- */
                if (pgid == 0) {
                    pgid = pid;          // first child becomes leader
                    leader_pid = pid;
                }
                setpgid(pid, pgid);      // ensure child joins group

                if (prev_pipe_read_end != STDIN_FILENO) close(prev_pipe_read_end);
                if (i != argc) {
                    close(pipefd[1]);
                    prev_pipe_read_end = pipefd[0];
                }
            }
            
            command_start_index = i + 1;
        }
    }

    // Wait for all child processes to finish
    int status;
    //while(wait(&status) > 0);
    while (waitpid(-pgid, &status, 0) > 0) {
        ; // reaping
    }
    /* Store job in table (optional: if foreground only) */
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = leader_pid;  // keep leader pid for messages
        jobs[job_count].pgid = pgid;       // store group id
        jobs[job_count].job_id = job_count + 1;
        jobs[job_count].command = strdup(argv[0]);
        jobs[job_count].state = 0; // Running
        job_count++;
    }
    return 0;
}
//#llm generated code ends