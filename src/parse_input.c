// #llm generated code begins
#include <string.h>
#include "parse_input.h"
#include<stdio.h>
#define MAX_LEN 256

int parse_name(char *token){
    // Inside parse_atomic(), right after checking argv[*pos] != NULL
    char *p=token;
    
    for (; *p; p++) {
        if (*p == '|' || *p == '&' || *p == '>' || *p == '<' || *p == ';' || *p == ' ') {
            return 0; // Invalid: contains special char inside a name token
        }
    }
    return 1;

}

int is_special(char *token) {
    return (strcmp(token, "&") == 0 || strcmp(token, ";") == 0 ||
            strcmp(token, "|") == 0 || strcmp(token, "<") == 0 ||
            strcmp(token, ">") == 0 || strcmp(token, ">>") == 0);
}

int consume_redirect(char *argv[], int *pos, int argc) {
    (*pos)++;
    //if (*pos >= argc || is_special(argv[*pos])) return 0; // Must be followed by filename
    if (!(*pos >= argc) && !parse_name(argv[*pos])) return 0;
    (*pos)++;
    return 1;
}

int parse_atomic(char *argv[], int *pos, int argc) {
    if (*pos >= argc) return 0;

    // Must start with a name (not a special symbol)
    if (is_special(argv[*pos])) return 0;
    if(!parse_name(argv[*pos])) return 0;
    (*pos)++;

    // Process optional args / redirections
    while (*pos < argc) {
        if (is_special(argv[*pos])) {
            // Input
            if (strcmp(argv[*pos], "<") == 0 ) {
                if (!consume_redirect(argv, pos, argc)) return 0;
            }
            // Output
            else if (strcmp(argv[*pos], ">") == 0 || strcmp(argv[*pos], ">>") == 0 ){
                if (!consume_redirect(argv, pos, argc)) return 0;
            }
            else {
                break; // Another operator belongs to higher-level parser
            }
        } else {
            if(!parse_name(argv[*pos])) return 0;
            (*pos)++; // Just another argument (name)
        }
    }
    return 1;
}

int parse_cmd_group(char *argv[],int *pos,int argc){
    if(!parse_atomic(argv ,pos,argc)){
        return 0;
    }
    while(*pos<argc && strcmp(argv[*pos],"|")==0){
        (*pos)++;
        if(*pos>=argc || !parse_atomic(argv, pos, argc)){
            return 0;
        }

    }
    return 1;
}

int parse_shell_cmd(char *argv[],int argc){
    int pos=0;

    if(!parse_cmd_group(argv,&pos,argc)){
        return 0;
    }
    while (pos < argc) {
        if (strcmp(argv[pos], "&") == 0 || strcmp(argv[pos], ";") == 0) {
            pos++;
            if (pos >= argc) {
                // The final '&' is valid. Any other operator at the end is invalid.
                if (strcmp(argv[pos - 1], "&") == 0) {
                    return 1;
                } else {
                    return 0;
                } 
            }
            if (!parse_cmd_group(argv, &pos, argc)) {
                return 0;
            }
        } else {
            break; // No more & or &&
        }
    }
    if (pos < argc && strcmp(argv[pos], "&") == 0) {
        pos++;
    }
    return pos == argc;
}

int parse_input(char *input, char *argv[],int max_args) {
    int argc = 0;
    const char *p = input;
    static char OP_AND[]  = "&";
    static char OP_SEMI[] = ";";
    static char OP_PIPE[] = "|";
    static char OP_LT[]   = "<";
    static char OP_GT[]   = ">";
    static char OP_GG[]   = ">>";
    while (*p && argc < max_args - 1) {
        // Skip whitespace
        while (*p == ' ' || *p == '\t') {
            p++; 
        }
        if (*p == '\0') break;

        if (*p == '&') {
            argv[argc++] = OP_AND;
            p++;
        }
        else if (*p == ';') {
            argv[argc++] = OP_SEMI;
            p++;
        }
        else if (*p == '|') {
            argv[argc++] = OP_PIPE;
            p++;
        }
        else if (*p == '<') {
            argv[argc++] = OP_LT;
            p++;
        }
        else if (*p == '>') {
            if (*(p+1) == '>') {
                argv[argc++] = OP_GG;
                p += 2;
            } else {
                argv[argc++] = OP_GT;
                p++;
            }
        }
        else {
            // NAME
            static char buf[MAX_LEN]; // static so pointer stays valid
            int c = 0;
            while (*p && *p!=' ' && *p!='&' && *p!=';' && *p!='|' && *p!='<' && *p!='>') {
                buf[c++] = *p++;
            }
            buf[c] = '\0';
            argv[argc++] = strdup(buf);  // copy so it doesn’t get overwritten
        }
    }

    argv[argc] = NULL;
    if(!parse_shell_cmd(argv,argc)){
        printf("Invalid Syntax!\n");
        return 0;
    }
    return argc;
}
//# llm generated code ends