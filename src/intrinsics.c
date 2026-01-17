//#llm generated code begins
#define _XOPEN_SOURCE 700 //for wcontinued

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include "intrinsics.h"
#include "execution.h" // for part e
#include <sys/wait.h> // wait pid and it's functions
#include <signal.h>
#include <libgen.h>

#define MAX_LOG_SIZE 15
#define SHELL_MAX_INPUT 1024
#define SHELL_MAX_ARGS 64

extern Job jobs[MAX_JOBS];
extern int job_count;
extern pid_t fg_pgid;



static char previous_dir[PATH_MAX] = "";
static char log_history[MAX_LOG_SIZE][SHELL_MAX_INPUT];
static int log_count = 0;
extern char shell_home_dir[PATH_MAX];

// Helper for qsort
static int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

// Function to handle reveal command
int reveal_cmd(int argc, char *argv[]) {
    bool show_all = false;
    bool line_by_line = false;
    char temp_target_path[PATH_MAX];
    char temp_current_dir[PATH_MAX];

    if (!getcwd(temp_current_dir, sizeof(temp_current_dir))) {
       fprintf(stderr, "No such directory!\n");
        return 1;
    }

    strcpy(temp_target_path, temp_current_dir);
    int i;
    int j;
    for (i = 1; i < argc; i++) {
        
        if (argv[i][0] == '-') { 
            for (j = 1; argv[i][j] != '\0'; j++) {
                if (argv[i][j] == 'a') {
                    show_all = true;
                } else if (argv[i][j] == 'l') {
                    line_by_line = true;
                }else{
                    printf("Invlaid Syntax!");
                    return 1;
                }
            }
        if(argv[i][1]=='\0'){
            break;
        }
        }else{
            break;
        }
    }
    if(i < argc){
        printf("%s argv[i] : %s\n",previous_dir,argv[i]);
        if (strcmp(argv[i], "~") == 0) {
            strcpy(temp_target_path, shell_home_dir);
        } else if (strcmp(argv[i], ".") == 0) {
            strcpy(temp_target_path, temp_current_dir);
        } else if (strcmp(argv[i], "..") == 0) {
            strcpy(temp_target_path,dirname(temp_current_dir));
        } else if (strcmp(argv[i],"-")==0){
            printf("hhi\n");
            if(previous_dir[0]=='\0'){
                printf("No such directory!\n");
                return 1;
            }
            strcpy(temp_target_path,previous_dir);
        }
        else {
            strcpy(temp_target_path, temp_current_dir);
            if(argv[i][0]!='/'){
                strcat(temp_target_path,"/");
            }
            strcat(temp_target_path,argv[i]);
        }
        i++;
    }
    if(i < argc){
        printf("Invalid Syntax!\n");
        return 1;
    }
    DIR *dir = opendir(temp_target_path);
    if (!dir) {
        fprintf(stderr,"No such directory!\n");
        return 1;
    }

    struct dirent *entry;
    char **file_list = malloc(sizeof(char *) * 1024);
    int file_count = 0;
    if (!file_list) {
        perror("malloc");
        closedir(dir);
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }
        file_list[file_count] = strdup(entry->d_name);
        file_count++;
    }
    closedir(dir);

    qsort(file_list, file_count, sizeof(char *), compare_strings);

    for (int i = 0; i < file_count; i++) {
        printf("%s", file_list[i]);
        if (line_by_line) {
            printf("\n");
        } else {
            printf(" ");
        }
        free(file_list[i]);
    }
    if (!line_by_line) {
        printf("\n");
    }

    free(file_list);
    return 0;
}

// Function to handle hop command
int hop_cmd(int argc, char *argv[]) {
    //struct passwd *pw = getpwuid(getuid());
    char current_dir[PATH_MAX];

    if (!getcwd(current_dir, sizeof(current_dir))) {
        fprintf(stderr,"No such directory!\n");
        return 1;
    }

    if (argc == 1 || (argc == 2 && strcmp(argv[1], "~") == 0)) {
        if (chdir(shell_home_dir) == 0) {
            strcpy(previous_dir, current_dir);
        } else {
            fprintf(stderr,"No such directory!\n");
            return 1;
        }
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], ".") == 0) {
            continue;
        } else if (strcmp(argv[i], "..") == 0) {
            if (chdir("..") == 0) {
                strcpy(previous_dir, current_dir);
                if (!getcwd(current_dir, sizeof(current_dir))) {
                    fprintf(stderr, "No such directory!\n");
                }
            } else {
                fprintf(stderr, "No such directory!\n");
            }
        } else if (strcmp(argv[i], "-") == 0) {
            if (previous_dir[0] == '\0') {
                continue;
            }
            char temp[PATH_MAX];
            strcpy(temp, current_dir);
            if (chdir(previous_dir) == 0) {
                strcpy(previous_dir, temp);
                if (!getcwd(current_dir, sizeof(current_dir))) {
                    fprintf(stderr, "No such directory!\n");
                }
            } else {
                perror("hop");
            }
        } else {
            if (chdir(argv[i]) == 0) {
                strcpy(previous_dir, current_dir);
                if (!getcwd(current_dir, sizeof(current_dir))) {
                    perror("getcwd");
                }
            } else {
                fprintf(stderr, "No such directory!\n");
            }
        }
    }
    return 0;
}

// Log management functions
void log_init() {
    FILE *log_file = fopen("history.txt", "r");
    if (log_file) {
        while (fgets(log_history[log_count], SHELL_MAX_INPUT, log_file) && log_count < MAX_LOG_SIZE) {
            log_history[log_count][strcspn(log_history[log_count], "\n")] = 0;
            log_count++;
        }
        fclose(log_file);
    }
}

void log_add(const char *cmd) {
    if (log_count > 0 && strcmp(log_history[log_count - 1], cmd) == 0) {
        return;
    }

    if (log_count == MAX_LOG_SIZE) {
        for (int i = 0; i < MAX_LOG_SIZE - 1; i++) {
            strcpy(log_history[i], log_history[i+1]);
        }
        log_count--;
    }
    strcpy(log_history[log_count], cmd);
    log_count++;

    FILE *log_file = fopen("history.txt", "w");
    if (log_file) {
        for (int i = 0; i < log_count; i++) {
            fprintf(log_file, "%s\n", log_history[i]);
        }
        fclose(log_file);
    }
}

int log_cmd(int argc, char *argv[]) {
    if (argc == 1) {
        for (int i = 0; i < log_count; i++) {
            printf("%s\n", log_history[i]);
        }
    } else if (strcmp(argv[1], "purge") == 0) {
        log_count = 0;
        FILE *log_file = fopen("history.txt", "w");
        if (log_file) {
            fclose(log_file);
        }
    } else if (strcmp(argv[1], "execute") == 0) {
        if (argc < 3) {
            fprintf(stderr, "log execute <index>\n");
            return 1;
        }
        int index = atoi(argv[2]);
        if (index <= 0 || index > log_count) {
            fprintf(stderr, "Invalid index\n");
            return 1;
        }

        char *cmd_to_exec = log_history[log_count - index];
        char *temp_cmd = strdup(cmd_to_exec);
        if (!temp_cmd) {
            perror("strdup");
            return 1;
        }

        // Re-tokenize and execute
        char *exec_argv[SHELL_MAX_ARGS];
        int exec_argc = 0;
        char *token = strtok(temp_cmd, " \t\n");
        while (token != NULL && exec_argc < SHELL_MAX_ARGS - 1) {
            exec_argv[exec_argc++] = token;
            token = strtok(NULL, " \t\n");
        }
        exec_argv[exec_argc] = NULL;
        
        check(exec_argc, exec_argv);

        free(temp_cmd);
    } else {
        fprintf(stderr, "Invalid log command\n");
    }
    return 0;
}

//For activities in part E --> here we make job functions in part d (execution) glogal

static int compare_jobs(const void *a, const void *b) {
    const Job *job_a = a;
    const Job *job_b = b;
    int cmp = strcmp(job_a->command, job_b->command);
    if (cmp == 0) {
        // if command names are equal, sort by PID
        if (job_a->pgid < job_b->pgid) return -1;
        if (job_a->pgid > job_b->pgid) return 1;
        return 0;
    }
    return cmp;
}

int activities(int argc , char *argv[]){
    Job sorted_jobs[MAX_JOBS];
    if(check_background_jobs()){
        int count=0;
        for (int i = 0; i < job_count; i++) {
            // check if process still exists
            if (kill(jobs[i].pid, 0) == 0) {
                sorted_jobs[count++] = jobs[i];
            }
        }

        qsort(sorted_jobs, count, sizeof(Job), compare_jobs);
        for(int i=0; i<count; i++){
            printf("[%d] %s -%s\n",sorted_jobs[i].pgid, sorted_jobs[i].command, sorted_jobs[i].state? "Stopped" : "Running");
        }
    }
    return 1;
}

int ping(int argc, char *argv[]){
    if(argc < 3){
        printf("Syntax is ping <pid> <signal_number>\n");
    }
    else{
        int pid=atoi(argv[1]);
        int signal_number=atoi(argv[2]);
        int actual_signal=signal_number%32;
        if(kill(pid,actual_signal)==0){
            
            printf("Sent signal %d to process with pid %d\n",signal_number,pid);
        }     
    }
    fflush(stdout);
    return 1;

}

/* ====================== fg ====================== */
int fg_cmd(int argc, char *argv[]) {
    int job_number = -1;
    if (argc > 1) {
        job_number = atoi(argv[1]);
    }

    Job *target = NULL;
    if (job_number == -1 && job_count > 0) {
        target = &jobs[job_count - 1];
    } else {
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].job_id == job_number) {
                target = &jobs[i];
                break;
            }
        }
    }

    if (!target) {
        printf("No such job\n");
        return 1;
    }

    printf("%s\n", target->command);
    fg_pgid = target->pgid;
    tcsetpgrp(STDIN_FILENO, target->pgid);

    if (target->state == 1) {
        kill(-target->pgid, SIGCONT); // CHANGED for pgid
        target->state = 0;
    }

    int status;
    pid_t w;
    do {
        w = waitpid(-target->pgid, &status, WUNTRACED);
        if (w == -1) break;
        if (WIFSTOPPED(status)) {
            target->state = 1; // stopped again
            break;
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    tcsetpgrp(STDIN_FILENO, getpgrp());
    fg_pgid = -1;
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        target->state = 2; // mark finished
    }
    return 0;
}

/* ====================== bg ====================== */
int bg_cmd(int argc, char *argv[]) {
    int job_number = -1;
    if (argc > 1) {
        job_number = atoi(argv[1]);
    }

    Job *target = NULL;
    if (job_number == -1 && job_count > 0) {
        target = &jobs[job_count - 1];
    } else {
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].job_id == job_number) {
                target = &jobs[i];
                break;
            }
        }
    }

    if (!target) {
        printf("No such job\n");
        return 1;
    }

    if (target->state == 0) {
        printf("Job already running\n");
        return 1;
    }

    kill(-target->pgid, SIGCONT); // CHANGED for pgid
    target->state = 0;
    printf("[%d] %s &\n", target->job_id, target->command);
    return 0;
}

// Function to check for and execute intrinsic commands
/*int check(int argc, char *argv[]){
    if (strcmp(argv[0], "hop") == 0) {
        hop_cmd(argc, argv);
        return 1;
    }
    else if (strcmp(argv[0], "reveal") == 0) {
        reveal_cmd(argc, argv);
        return 1;
    }
    else if (strcmp(argv[0], "log") == 0) {
        log_cmd(argc, argv);
        return 1;
    }
    else if (strcmp(argv[0], "activities") == 0){
        activities(argc,argv);
        return 1;
    }
    else if (strcmp(argv[0], "ping") == 0){
        ping(argc,argv);
        return 1;
    }
    return 2;
}*/
// intrinsics.c
// ...

// New dispatcher function to handle intrinsics that can be redirected
void run_intrinsic(int argc, char *argv[]) {
    if (strcmp(argv[0], "reveal") == 0) {
        reveal_cmd(argc, argv);
    } else if (strcmp(argv[0], "log") == 0) {
        log_cmd(argc, argv);
    } else if (strcmp(argv[0], "activities") == 0) {
        activities(argc, argv);
    } else if (strcmp(argv[0], "ping") == 0) {
        ping(argc, argv);
    }
}

// Check now only identifies the type of command
// Returns 0 for 'hop', 1 for other intrinsics, 2 for externals
int check(int argc, char *argv[]){
    if (strcmp(argv[0], "hop") == 0 || strcmp(argv[0], "fg")==0 || strcmp(argv[0], "bg") == 0) {
        return 0;
    } else if (strcmp(argv[0], "reveal") == 0 ||
               strcmp(argv[0], "log") == 0 ||
               strcmp(argv[0], "activities") == 0 ||
               strcmp(argv[0], "ping") == 0 ) {
        return 1;
    }
    return 2;
}
//#llm generated code ends