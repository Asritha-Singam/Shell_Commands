// execution.h
//----#llm generated code begin
#ifndef EXECUTION_H
#define EXECUTION_H
#define MAX_JOBS 64
#define SHELL_MAX_ARGS 64
int check_background_jobs();
void execute_commands(char **argv, int argc);

typedef struct {
    pid_t pid;
    pid_t pgid;
    char *command;
    int job_id;
    int state; //for E
} Job;
extern Job jobs[MAX_JOBS];
extern int job_count;

extern pid_t fg_pgid;
#endif
// ---#llm generated code end