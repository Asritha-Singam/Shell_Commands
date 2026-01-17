//---#llm generated code begin
#ifndef INTRINSICS_H
#define INTRINSICS_H

int hop_cmd(int argc, char *argv[]);
int reveal_cmd(int argc, char *argv[]);
int log_cmd(int argc, char *argv[]);
int check(int argc, char *argv[]);
void log_init();
void log_add(const char *cmd);

int activities(int argc , char *argv[]);
int ping(int argc, char *argv[]);
int fg_cmd(int argc, char *argv[]);
int bg_cmd(int argc, char *argv[]);
void run_intrinsic(int argc, char *argv[]);

#endif
//#llm generated code end