/**
 * @file     pa1.c
 * @author   Apykhin Artem & Grechukhin Kirill
 * @date     September, 2023
 * @brief    C library for interprocess interaction
 */

#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include "pa1_custom.h"

static const char * const events_log;
static const char * const pipes_log;

static const char * const log_started_fmt;
static const char * const log_received_all_started_fmt;
static const char * const log_done_fmt;
static const char * const log_received_all_done_fmt;

static const char * const pipe_closed_read_from_for;
static const char * const pipe_closed_write_from_into;
static const char * const pipe_opened;

#define im_not_a_father father.my_pid != getpid()

static char buffer[50] = ""; 

void make_a_pipes(int32_t children_number, int pipes_file);
void leave_needed_pipes(local_id id, int32_t children_number);
void close_rest_of_pipes(local_id id, int32_t children_number);
void close_pipe_file();
//int test_send(void * self, local_id dst, int *number);
//int test_receive(void * self, local_id from, int *number);

// initialization 
void actor_dad(struct Actor *dad, pid_t zero_dad, int32_t children_number){
    dad->my_id = PARENT_ID;
    dad->my_pid = zero_dad;
    dad->my_father_pid = -1;
    //dad->my_role = DAD;
    dad->my_kids = children_number;
    dad->my_sisters = 0;
    dad->my_balance = 0;
}

void actor_daughter(struct Actor *daughter, local_id id, pid_t pid, pid_t father_pid, int32_t children_number, uint8_t balance) {
    daughter->my_id = id;
    daughter->my_pid = pid;
    daughter->my_father_pid = father_pid;
    //daughter->my_role = CHILD;
    daughter->my_kids = 0;
    daughter->my_sisters = children_number-1;
    daughter->my_balance = 0;
    sprintf(buffer, log_started_fmt, id, pid, father_pid);
    printf(log_started_fmt, id, pid, father_pid);
}

// pid named dad make fork kids times 
void become_a_dad(struct Actor *dad, struct Actor *daughter, int32_t children_number){
    //first_daughter
    uint8_t fork_result = fork();
    local_id id = PARENT_ID + 1;
    if (fork_result == 0) {
        actor_daughter(daughter, id, getpid(), dad->my_pid, children_number, 0);
        return; 
    }
    
    //rest daughters
    for(uint8_t i = 1; i < dad->my_kids; i++){
        if (fork_result > 0) {
            fork_result = fork();
            id++;
        } 
        if (fork_result == 0 && i == dad->my_kids - 1){
            actor_daughter(daughter, id, getpid(), dad->my_pid, children_number, i);
            return;
        }
    }
}

Message make_a_message(MessageType messageType, const char *message) {
    Message msg;

    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = strlen(message);
    msg.s_header.s_local_time = time(NULL);
    msg.s_header.s_type = messageType;
    // msg.s_header.s_local_time = get_physical_time();
    // А нафига это?
    // switch (messageType) {
    //     case STARTED:
    //         msg.s_header.s_type = STARTED;
    //         break;
    //     case DONE:
    //         msg.s_header.s_type = DONE;
    //         break;
    //     default:
    //         msg.s_header.s_type = ACK; 
    //         break;
    // }

    strncpy(msg.s_payload, message, sizeof(msg.s_payload));
    
    return msg;
}



int prepare_for_work(struct Actor *dad, struct Actor *daughter){
    // Message msg = make_a_message(STARTED, buffer);
    Message msg = make_a_message(STARTED, "");
    if (send_multicast(daughter, &msg) != 0){
        printf("Write error\n");
        return 1;
    }
    
    int counter = 0;
    while (counter < 1000) {
        int32_t recievers;
        recievers = daughter->my_sisters+1;
        for (int32_t i = 1; i <= recievers; i++) {
            if (daughter->my_id != i) {
                if (receive(daughter, i, &msg) != 0) {
                    return -1;
                }
            }
        }
        counter++;
        int recivied_started = 0;
        for (int i = 1; i <= (daughter->my_sisters)+1; i++) {
            if (last_recieved_message[i] == STARTED && i != daughter->my_id) {
                recivied_started++;
            }
        }
    
        if (recivied_started == daughter -> my_sisters) {
            sprintf(buffer, log_received_all_started_fmt, daughter->my_id);
            printf(log_received_all_started_fmt, daughter->my_id);
            return 0;
        }
    }
    if (counter == 1000) {
        printf("Out of try\n");
        return -1;
    }
    return -1;
}


int at_work(struct Actor *dad, struct Actor *daughter){
    //Some kind of work
    // if (daughter->my_id != 1) {
    //     Message some_message = make_a_message(CS_REPLY, buffer);
    //     printf("Process %d starting to receive testing message\n", daughter->my_id);
    //     receive(daughter, 1, &some_message);
    //     if (some_message.s_header.s_type == CS_REPLY) {
    //         printf("Process %d received what he sent\n", daughter->my_id);
    //     } else if (some_message.s_header.s_type == DONE) {
    //         printf("Process %d received DONE... somehow\n", daughter->my_id);
    //     } else if (some_message.s_header.s_type == ACK) {
    //         printf("Process %d received ACK. Shet\n", daughter->my_id);  
    //     }  
    //     else {
    //         printf("Process %d received some shit\n", daughter->my_id);
    //     }
    //     printf("Process %d received %d\n", daughter->my_id, some_message.s_header.s_type);
        
    // }
    // Message some_message = make_a_message(ACK, "");
    // while (some_message.s_header.s_type != STOP) {
    //     while (receive_any(daughter, &some_message) != 0) {
    //         // printf("%s\n", "still receiving");
    //     }
    //     // printf("%s\n", "succed receive_any");
    //     if (some_message.s_header.s_type == TRANSFER) {
    //         printf("%s\n", some_message.s_payload);
    //     }
    // }
    return 0;
}


int before_a_sleep(struct Actor *dad, struct Actor *daughter){
    sprintf(buffer, log_done_fmt, daughter->my_id);
    printf(log_done_fmt, daughter->my_id);
    // Message done_message = make_a_message(DONE, buffer);
    Message done_message = make_a_message(DONE, "");
    if (send_multicast(daughter, &done_message) != 0) {
        printf("Write error\n");
        return 1;
    }
    int counter = 0;
    while (counter < 1000) {
        int32_t recievers= daughter->my_sisters+1;
        for (int32_t i = 1; i <= recievers; i++) {
            if (daughter->my_id != i) {
                if (receive(daughter, i, &done_message) != 0) {
                    return -1;
                }
            }
        }
        counter++;
        int recivied_done = 0;
        for (int i = 1; i <= (daughter->my_sisters)+1; i++) {
            if (last_recieved_message[i] == DONE && i != daughter->my_id)
                recivied_done++;
        }
        if (recivied_done == daughter -> my_sisters) {
            sprintf(buffer, log_received_all_done_fmt, daughter->my_id);
            printf(log_received_all_done_fmt, daughter->my_id);
            return 0;
        }
    }
    if (counter == 1000) {
        printf("Out of try\n");
        return -1;
    }
    return -1;
}

int father_check_started(struct Actor *dad) {
    // Message msg = make_a_message(STARTED, buffer);
    Message msg = make_a_message(STARTED, "");
    int counter = 0;
    while (counter < 1000) {
        int32_t recievers = dad->my_kids;
        for (int32_t i = 1; i <= recievers; i++) {
            if (dad->my_id != i) {
                if (receive(dad, i, &msg) != 0) {
                    return -1;
                }
            }
        }
        counter++;
        int recivied_started = 0;
        for (int i = 1; i <= (dad->my_kids); i++) {
            if (last_recieved_message[i] == STARTED && i != PARENT_ID)
                recivied_started++;
        }
        if (recivied_started == dad->my_kids)
            return 0;
        }
    if (counter == 1000) {
        printf("Out of try\n");
        return -1;
    }
    
    return -1;
}

int father_want_some_sleep(struct Actor *dad) {
    // Message done_message = make_a_message(DONE, buffer);
    Message done_message = make_a_message(DONE, "");
    int counter = 0;
    while (counter < 1000) {
        int32_t recievers = dad->my_kids;
        for (int32_t i = 1; i <= recievers; i++) {
            if (dad->my_id != i) {
                if (receive(dad, i, &done_message) != 0) {
                    return -1;
                }
            }
        }
        counter++;
        int recivied_done = 0;
        for (int i = 1; i <= (dad->my_kids); i++) {
            if (last_recieved_message[i] == DONE && i != PARENT_ID)
                recivied_done++;
        }
        if (recivied_done == dad->my_kids) 
            return 0;
        }
    if (counter == 1000) {
        printf("Out of try\n");
        return -1;
    }
    return -1;
}

// int main(int argc, char *argv[]) {
//     for (int i = 0; i < 16; i++) {
//         last_recieved_message[i] = STOP;
//     }
//     // Check for parameters 
//     int32_t children_number;
//     if (argc != 3 || strcmp(argv[1], "-p") != 0 || atoi(argv[2]) == 0){
//         // printf("Неопознанный ключ или неверное количество аргументов! Пример: -p <количество процессов>\n");
//         exit(1);
//     }
//     else{
//         if (atoi(argv[2]) > 9 || atoi(argv[2]) <= PARENT_ID){
//             // printf("Некорректное количество процессов для создания (0 < x <= 9)!\n");
//             exit(1);
//         }   
//         else
//             children_number = atoi(argv[2]);
//     }

//     //Try to create or open files
//     int events_file = open(events_log, O_WRONLY | O_APPEND | O_CREAT);
//     int pipes_file = open(pipes_log, O_WRONLY | O_APPEND | O_CREAT); 
//     // int events_file = open(events_log, O_WRONLY | O_TRUNC | O_CREAT);
//     // int pipes_file = open(pipes_log, O_WRONLY | O_TRUNC | O_CREAT); 
//     if (events_file == -1)
//         exit(1);
//     if (pipes_file == -1)
//         exit(1);

//     //Make pipes
//     make_a_pipes(children_number, pipes_file);
//     close(pipes_file);

//     //The appearance of the father
//     struct Actor father;
//     actor_dad(&father, getpid(), children_number);

//     //Start of fork (born of daughters)
//     struct Actor daughter;
//     become_a_dad(&father, &daughter, children_number);

//     if (im_not_a_father){
//         write(events_file, buffer, strlen(buffer));

//         leave_needed_pipes(daughter.my_id, children_number); //Close extra pipes for child processes
        
//         if (prepare_for_work(&father, &daughter) == 0) //Synchronization before useful work
//             write(events_file, buffer, strlen(buffer)); 
//         else
//             exit(1);

//         if (at_work(&father, &daughter) == 0) //Useful work
//             write(events_file, buffer, strlen(buffer));
//         else
//             exit(1);
        

//         if (before_a_sleep(&father, &daughter) == 0) //Synchronization after useful work
//             write(events_file, buffer, strlen(buffer));
//         else
//             exit(1);
        
//         close_rest_of_pipes(daughter.my_id, children_number);
//         close(events_file);
//         close_pipe_file();
//         exit(0);
//     }
//     else{
//         leave_needed_pipes(father.my_id, children_number); //Close extra pipes for main processes
//         int status;
//         while (father_check_started(&father) != 0)continue;

//         while (children_number != 0) 
//             if ((status = waitpid(-1, NULL, WEXITED || WNOHANG)) <= 0)children_number--;

//         while(father_want_some_sleep(&father) != 0) continue;
//         close_rest_of_pipes(PARENT_ID, father.my_kids);
//         //Waiting for the end of child processes 
        
//         //Closing of files
//         close(events_file);
//         close_pipe_file();
//     } 
//     return 0;
// }
