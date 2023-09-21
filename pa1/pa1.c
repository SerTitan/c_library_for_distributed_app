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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define im_not_a_father father.my_pid != getpid()

//Файлы логово и сообщений для пайпов
static const char * const events_log;
static const char * const pipes_log;

static const char * const log_started_fmt;
static const char * const log_received_all_started_fmt;
static const char * const log_done_fmt;
static const char * const log_received_all_done_fmt;

char buffer[100];
int fd[45][2];

struct Actor {
    enum _role{
        DAD,
        WAITING_TO_BE_DED,
        DAD_IS_DED, // After all child becomes students 
        CHILD,
        STUDENT //After sent all messages
    };
    local_id my_id;
    pid_t my_pid;
    pid_t my_father_pid;
    uint8_t my_kids;
    enum _role my_role;
};

// initialization 
void actor_dad(struct Actor *dad, uint8_t embryo, pid_t zero_dad){
    dad->my_id = PARENT_ID;
    dad->my_pid = zero_dad;
    dad->my_father_pid = -1;
    dad->my_role = DAD;
    dad->my_kids= embryo;
}

void actor_daughter(struct Actor *daughter, local_id id, pid_t pid, pid_t father_pid) {
    daughter->my_id = id;
    daughter->my_pid = pid;
    daughter->my_father_pid = father_pid;
    daughter->my_role = CHILD;
    daughter->my_kids = 0;
    //printf(log_started_fmt, id, pid, father_pid);
    sprintf(buffer, log_started_fmt, id, pid, father_pid);
}



// pid named dad make fork kids times 
void become_a_dad(struct Actor *dad, struct Actor *daughter){
    //first_daughter
    uint8_t fork_result = fork();
    local_id id = PARENT_ID + 1;
    if (fork_result == 0) {
        actor_daughter(daughter, id, getpid(), dad->my_pid);
        return; 
    }
    
    //rest daughters
    for(uint8_t i = 1; i < dad->my_kids; i++){
        if (fork_result > 0) {
            fork_result = fork();
            id++;
        } 
        if (fork_result == 0 && i == dad->my_kids - 1){
            actor_daughter(daughter, id, getpid(), dad->my_pid);
            return;
        }
    }
}


int send(void * self, local_id dst, const Message * msg) {
    // TODO
    // struct Actor *daughter = (struct Actor *)self;
    // write(fd[daughter->my_id+dst][1], msg, sizeof(Message));
    return 0;
}

int send_multicast(void * self, const Message * msg) {
    // TODO
    return 0;
}

int receive(void * self, local_id from, Message * msg) {
    //read(fd[from][0], );
    return 0;
}

int receive_any(void * self, Message * msg){
    // TODO
    return 0;
}

/*
0: 0 ... children_number-2
1: children_number-1 ... 2*children_number - 1
2: 
children_number-1: children_number*(children_number-1) ... 
4
0: 0, 1, 2, 3
1: 4, 5, 6
2: 7, 8
3: 9
4:
*/

//CLose unused pipes
void leave_needed_pipes(local_id id, uint8_t children_number) {
    // fd[x][0] - read from x 
    // fd[x][1]
    // id == 2
    for (int8_t i = 0; i < children_number; i++) {
        for (int8_t j = 0; j < children_number-i; j++) {
            if (i != id) {
                if (i > id || j + i != id) {
                    close(fd[i*children_number+j][0]);
                    close(fd[i*children_number+j][1]);
                }
            }
        }
    }
}


int main(int argc, char *argv[]) {

    // Check for parameters 
    uint8_t children_number = 0;
    if (argc != 3 || strcmp(argv[1], "-p") != 0 || atoi(argv[2]) == 0){
        exit(1);
    }
    else{
        if (atoi(argv[2]) > 10 || atoi(argv[2]) < 0)
            exit(1);
        else
            children_number = atoi(argv[2]);
    }

    //Try to create or open files
    int f1 = open(events_log, O_WRONLY, O_TRUNC | O_CREAT);
    int f2 = open(pipes_log, O_WRONLY, O_TRUNC | O_CREAT); 
    if (f1 == -1)
        exit(1);
    if (f2 == -1)
        exit(1);

    //Make pipes
    for (int i = 0; i < children_number*(children_number-1)/2; i++) {
        pipe(fd[i]);
    }

    //The appearance of the father
    struct Actor father;
    actor_dad(&father, children_number, getpid());

    //Start of fork (born of daughters)
    struct Actor daughter;
    become_a_dad(&father, &daughter);
    
    //Close extra pipes
    if (im_not_a_father) 
        leave_needed_pipes(daughter.my_id, children_number);
    else
        leave_needed_pipes(father.my_id, children_number);
    
    //Logging process working
    if (im_not_a_father)
        write(f1, buffer, strlen(buffer));

    if (im_not_a_father){
        sprintf(buffer, log_received_all_done_fmt, daughter.my_id);
        write(f1, buffer, strlen(buffer));
    }

    //Close of files
    if (!im_not_a_father){
        close(f1);
        close(f2);
    }
    
    return 0;
}
