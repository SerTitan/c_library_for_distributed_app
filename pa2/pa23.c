/**
 * @file     pa23.c
 * @author   Apykhin Artem & Grechukhin Kirill
 * @date     November, 2023
 * @brief    C library for interprocess interaction
 */

#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include "pa1_custom.h"
#include "banking.h"


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
void actor_dad(struct Actor *dad, pid_t zero_dad, int32_t children_number);
void actor_daughter(struct Actor *daughter, local_id id, pid_t pid, pid_t father_pid, int32_t children_number);
void become_a_dad(struct Actor *dad, struct Actor *daughter, int32_t children_number);
int prepare_for_work(struct Actor *dad, struct Actor *daughter);
int at_work(struct Actor *dad, struct Actor *daughter);
int before_a_sleep(struct Actor *dad, struct Actor *daughter);
int father_check_started(struct Actor *dad);
int father_want_some_sleep(struct Actor *dad);
// int receive(void * self, local_id from, Message * msg);
// int send(void * self, local_id dst, const Message * msg);
Message make_a_message(MessageType messageType, const char *message);

TransferOrder makeTransferOrder(local_id s_src, local_id s_dst, balance_t s_amount) {
    TransferOrder order;
    order.s_amount = s_amount;
    order.s_dst = s_dst;
    order.s_src = s_src;

    return order;
}

void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount) {
    struct Actor *father = (struct Actor *)parent_data;
    TransferOrder order = makeTransferOrder(src, dst, amount);
    char order_string[100];
    sprintf(order_string, "%d %d %d", order.s_src, order.s_dst, order.s_amount);
    // printf("%s\n", order_string);
    Message to_src_message = make_a_message(TRANSFER, order_string);
    // printf("%d\n", strlen(order_string));
    send(father, src, &to_src_message);
    send(father, dst, &to_src_message);
    receive(father, dst, &to_src_message);
}

int main(int argc, char * argv[]) {
    for (int i = 0; i < 16; i++) {
        last_recieved_message[i] = STOP;
    }
    // Check for parameters 
    int32_t children_number;
    uint8_t bank_account[argc-3];
    if (strcmp(argv[1], "-p") != 0 || atoi(argv[2]) == 0 || atoi(argv[2]) != argc-3){
        printf("%s %s %d %d\n", argv[1], argv[2],argc-3, strcmp(argv[1], "-p"));
        exit(2);
    }
    else{
        if (atoi(argv[2]) > 10 || atoi(argv[2]) <= PARENT_ID){
            //printf("Некорректное количество процессов для создания (0 < x <= 9)!\n");
            exit(3);
        }   
        else {
            children_number = atoi(argv[2]);
            size_t i;
            for (i = 1; i <= children_number; i++){
                if (atoi(argv[2+i]) > 0 && atoi(argv[2+i]) < 100) 
                    bank_account[i-1] = atoi(argv[2+i]);
                else {
                    //printf("Некорректный счет для %zu дочери (1 < x <= 99)!\n", i);
                    exit(4);
                }
            }
        } 
    }

    //Try to create or open files
    int events_file = open(events_log, O_WRONLY | O_APPEND | O_CREAT);
    int pipes_file = open(pipes_log, O_WRONLY | O_APPEND | O_CREAT); 
    // int events_file = open(events_log, O_WRONLY | O_TRUNC | O_CREAT);
    // int pipes_file = open(pipes_log, O_WRONLY | O_TRUNC | O_CREAT); 
    if (events_file == -1)
        exit(1);
    if (pipes_file == -1)
        exit(1);

    //Make pipes
    make_a_pipes(children_number, pipes_file);
    close(pipes_file);

    //The appearance of the father
    struct Actor father;
    actor_dad(&father, getpid(), children_number);

    //Start of fork (born of daughters)
    struct Actor daughter;
    become_a_dad(&father, &daughter, children_number);

    if (im_not_a_father){
        write(events_file, buffer, strlen(buffer));

        leave_needed_pipes(daughter.my_id, children_number); //Close extra pipes for child processes
        
        if (prepare_for_work(&father, &daughter) == 0) //Synchronization before useful work
            write(events_file, buffer, strlen(buffer)); 
        else
            exit(1);

        if (at_work(&father, &daughter) == 0) //Useful work
            write(events_file, buffer, strlen(buffer));
        else
            exit(1);
        

        if (before_a_sleep(&father, &daughter) == 0) //Synchronization after useful work
            write(events_file, buffer, strlen(buffer));
        else
            exit(1);
        
        close_rest_of_pipes(daughter.my_id, children_number);
        close(events_file);
        close_pipe_file();
        exit(0);
    }
    else{
        leave_needed_pipes(father.my_id, children_number); //Close extra pipes for main processes
        int status;
        while (father_check_started(&father) != 0)continue;
        // transfer(&father, 1, 2, 10);
        //TODO
        //bank_robbery(parent_data);
        // Message stopMessage = make_a_message(STOP, "");
        // send_multicast(&father, &stopMessage);
        while (children_number != 0) 
            if ((status = waitpid(-1, NULL, WEXITED || WNOHANG)) <= 0)children_number--;

        while(father_want_some_sleep(&father) != 0) continue;
        close_rest_of_pipes(PARENT_ID, father.my_kids);
        //Waiting for the end of child processes 
        
        //Closing of files
        close(events_file);
        close_pipe_file();
        //print_history(all);
    } 

    return 0;
}
