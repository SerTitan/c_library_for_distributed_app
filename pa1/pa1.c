/**
 * @file     pa1.c
 * @Author   Apykhin Artem & Grechukhin Kirill
 * @date     September, 2023
 * @brief    C library for interprocess interaction
 */

#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>

//Файлы логово и сообщений для пайпов
static const char * const events_log;
static const char * const pipes_log;

static const char * const log_started_fmt;

struct Actor {
    MessageType my_message;
    enum _role{
        DAD,
        WAITING_TO_BE_DED,
        DAD_IS_DED,
        CHILD,
        STUDENT
    };
    local_id my_id;
    pid_t my_pid;
    pid_t my_father_pid;
    local_id my_kids;
    enum _role my_role;
};

/*
void actor_fork(){

};
*/

void become_a_dad(struct Actor *dad){
    for(local_id i = 0; i<=dad->my_kids; i++){
        pid_t test_tube = getpid();
        if (test_tube == dad->my_pid) 
            fork();
        if (i == dad->my_kids && test_tube != dad->my_pid)
            printf("I was born with serial number on my sleeve %d\n", getpid());
    }
}

void actor_dad(struct Actor *dad, local_id embryo, pid_t zero_dad){
    dad->my_id = PARENT_ID;
    dad->my_pid = zero_dad;
    dad->my_father_pid = -1;
    dad->my_role = DAD;
    dad->my_kids= embryo;
}

int main(int argc, char *argv[]) {
    // Check for parameters 
    local_id children_number = 0;
    if (argc != 3 || strcmp(argv[1], "-p") != 0 || atoi(argv[2]) == 0){
        exit(1);
    }
    else{
        if (atoi(argv[2]) > 9 || atoi(argv[2]) < 0)
            exit(1);
        else
            children_number = atoi(argv[2]);
    }
    // End of check

    //Try to open files
    FILE *events_file = fopen(events_log, "r");
    FILE *pipes_file = fopen(pipes_log, "r"); 
    if (!events_file) {
         events_file = fopen(events_log, "w");
        if (events_file)
            fclose(events_file);
        else
            exit(1);
    }
    if (!pipes_file) {
         pipes_file = fopen(pipes_log, "w");
        if (pipes_file)
            fclose(pipes_file);
        else
            exit(1);
    }
    //End of try


    //Start of fork
    struct Actor father;
    actor_dad(&father, children_number, getpid());
    become_a_dad(&father);
    //End of fork
    
    return 0;
}
