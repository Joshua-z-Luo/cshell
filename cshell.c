#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define MAX 1000
#define TOKSIZE 128

#define DELIMITER " \t\r\n\a" 

typedef struct {
    char *name;
    char *value;
} EnvVar;

EnvVar *vars;
int var_max;
int var_pos;

typedef struct {
    char *name;
    struct tm *time;
    int ret;
} Command;

Command *logs;
int log_pos;
int log_max;

    
char **tokenize(char *line){

    int toksize = TOKSIZE;
    char *token;
    char **tokens = (char**) malloc(toksize * sizeof(char*));

    int pos = 0;

    token = strtok(line, DELIMITER);
    while(token != NULL){
        tokens[pos] = token;
        pos++;

        if (pos >= toksize) {
            toksize = (int)(toksize*2);
            tokens = realloc(tokens, toksize * sizeof(char*));
        }
        token = strtok(NULL, DELIMITER);
    }
    tokens[pos] = NULL;
    return tokens;
}

char* input() {
    char *line = NULL;
    size_t max = 0;
    getline(&line, &max, stdin);
    return line;
}





void logupdate(char **tokens){
    int name_len = strlen(tokens[0]);
    char* name = (char*) malloc(sizeof(char)*name_len);
    strcpy(name, tokens[0]);
    logs[log_pos].name = name;

    time_t rawtime;
    time(&rawtime);
    logs[log_pos].time = localtime(&rawtime);


    if (log_pos >= log_max) {
        log_max = (int)(log_max*2);
        logs = realloc(logs, log_max * sizeof(Command));
    }
}

void logupdate_env(char *token){
    int name_len = strlen(token);
    char* name = (char*) malloc(sizeof(name_len));
    strcpy(name, token);
    logs[log_pos].name = name;

    time_t rawtime;
    time(&rawtime);
    logs[log_pos].time = localtime(&rawtime);


    if (log_pos >= log_max) {
        log_max = (int)(log_max*2);
        logs = realloc(logs, log_max * sizeof(Command));
    }
}

int log(char **tokens){
    logupdate(tokens);
    if(tokens[1]!=NULL){
        printf("too many args\n");
    }

    if (log_pos == 0){
        printf("logs is empty\n");
    }
    for(int i = 0; i < log_pos; i++){
        printf("%s", asctime(logs[i].time));
        printf("%s ", logs[i].name);
        printf("%d\n", logs[i].ret);
    }
    return 1;
}

int customprint(char **tokens) {
    logupdate(tokens);
    int itr = 1;
    if(tokens[itr]==NULL){
        printf("nothing to print\n");
        logs[log_pos].ret = -1;
        log_pos++;
        return 1;
    }

    while(tokens[itr]!=NULL){
        int var_found = 0;
        if(tokens[itr][0]=='$'){
            for(int i = 0; i < var_pos; i++){
                if (strcmp(vars[i].name, tokens[itr])==0){
                    printf("%s ", vars[i].value);
                    var_found = 1;
                    break;
                }
            }
            if(var_found==0){
                printf("%s not found as enviroment variable\n", tokens[itr]);
                logs[log_pos].ret = -1;
                log_pos++;
                return 0;

            }
        } else
        printf("%s ", tokens[itr]);
        itr++;
    }
    printf("\n");
    logs[log_pos].ret = 0;
    log_pos++;
    return 1;
}

void execute(char **tokens) {
    if(!strcmp(tokens[0], "print")){
        customprint(tokens);
    } else if(!strcmp(tokens[0], "log")){
        log(tokens);
        int test;
    } else if(!strcmp(tokens[0], "theme")){
        theme(tokens);
    } else{



        pid_t pid = fork();
        int status;

        logupdate(tokens);
        //child
        if(pid==0){
            if(execvp(tokens[0], tokens)==-1){
                printf("command not found\n");
                logs[log_pos].ret = -1;
                log_pos++;
                
            }
            exit(1);
        } else if(pid<0){
            printf("fork failed\n");
            exit(1);

        } else{
            //parent
            wait(&status);
            logs[log_pos].ret = status;
            log_pos++;
            

        }
    }
}



int envar(char *line) {
    logupdate_env(line);
    int has_space = 0;
    for(int i=0; i<strlen(line);i++){
        if(line[i]==' '){
            has_space = 1;
            break;
        }
    }
    if(has_space){
        printf("improper format for setting EnvVar\n");
        logs[log_pos].ret = -1;
        log_pos++;
    }


    char **nameval = (char**)(malloc(sizeof(char*) * 1024));
    nameval[0] = strtok(line, "=");
    nameval[1] = strtok(NULL, "=\n");

    int found = 0;
    for(int i = 0; i < var_pos; i++){
        if(strcmp(vars[i].name, nameval[0])==0) {
            vars[i].value = nameval[1];
            found = 1;
            var_pos++;
            logs[log_pos].ret = 0;
            log_pos++;
            return 0;
        }
    }
    if(found==0){
        vars[var_pos].name = (char*) malloc(sizeof(char)*1024);
        vars[var_pos].value = (char*) malloc(sizeof(char)*1024);
        strcpy(vars[var_pos].name, nameval[0]);
        strcpy(vars[var_pos].value, nameval[1]);
    }
    
    var_pos++;
    if (var_pos >= var_max) {
            var_max = (int)(var_max*2);
            vars = (EnvVar*) realloc(vars, var_max);
        }

    logs[log_pos].ret = 0;
    log_pos++;
    return 1;
}





int theme(char **tokens) {
    logupdate(tokens);
    if(tokens[1]==NULL){
        printf("unsupported theme\n");
        logs[log_pos].ret = -1;
        log_pos++;
        return 0;
    }
    if(strcmp(tokens[1], "blue")==0){
        printf("\033[0;34m");
    }else if(strcmp(tokens[1], "red")==0){
        printf("\033[0;31m");
    }else if(strcmp(tokens[1], "green")==0){
        printf("\033[0;32m");
    }else{
        printf("unsupported theme\n");
        logs[log_pos].ret = -1;
        log_pos++;
        return 0;
    }
    logs[log_pos].ret = 0;
    log_pos++;
    return 1;
}

int main(int argc, char** argv){

    int var_max = 16;
    int var_pos = 0;
    vars = (EnvVar*) malloc(var_max * sizeof(EnvVar));

    log_pos = 0;
    log_max = 16;
    logs = (Command*) malloc(log_max*sizeof(Command));
    
    //interactive mode
    if(argc==1){
        while(1){
            printf("cshell$ ");
            //get input
            char *line = input();
            if(line[0]=='$'){
                envar(line);
            }else{
                //split into tokens
                char **tokens = tokenize(line);

                if(tokens[0]!=NULL){
                    if(strcmp(tokens[0], "exit")==0){
                        printf("Bye!\n");
                        break;
                    }
                    execute(tokens);
                }

            }
  
            }
    }
    //script mode
    else if(argc == 2){
        FILE* f = fopen(argv[1], "r");
        if (f == NULL){
            printf("error opening file\n");
            return 0;
        }
        char *line[1024];
        while (fgets(line, 1024, f)){
            //get input
            if(line[0]=='$'){
                envar(line);
            }else{
                //split into tokens
                char **tokens = tokenize(line);

                if(tokens[0]!=NULL){
                    if(strcmp(tokens[0], "exit")==0){
                        printf("Bye!\n");
                        break;
                    }
                    execute(tokens);
                }

            }

        }
        fclose(f);
    }
        
    
}
