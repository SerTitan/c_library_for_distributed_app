/**
 * @file     pa23.c
 * @author   Apykhin Artem & Grechukhin Kirill
 * @date     November, 2023
 * @brief    C library for interprocess interaction
 */

#include "common.h"
#include "ipc.h"
#include "pa1_custom.h"
#include "pa2345.h"
#include "banking.h"

static const char * const events_log;
static const char * const pipes_log;

static const char * const pipe_closed_read_from_for;
static const char * const pipe_closed_write_from_into;
static const char * const pipe_opened;

#define im_not_a_father father.my_pid != getpid()

//Creation and closing pipes
void make_a_pipes(int32_t children_number);
void leave_needed_pipes(local_id id, int32_t children_number);
void close_rest_of_pipes(local_id id, int32_t children_number);

//For use events_file in pa1.c file
void events_f_to_pa1(int events_file);
void close_events_file();

//Born of daughters
void actor_dad(struct Actor *dad, pid_t zero_dad, int32_t children_number);
void actor_daughter(struct Actor *daughter, local_id id, pid_t pid, pid_t father_pid, int32_t children_number, balance_t balance);
void become_a_dad(struct Actor *dad, struct Actor *daughter, int32_t children_number, balance_t daughter_bank_account[]);

//Stages of daughter's working process
int started_to_send(struct Actor *daughter);
int prepare_for_work(struct Actor *daughter);
int at_work(struct Actor *daughter);
int done_to_send(struct Actor *daughter);
int before_a_sleep(struct Actor *daughter);
int check_recieve_again(struct Actor *daughter);
int send_balance_history(struct Actor *daughter);

//Stages of fathers's working process
int father_check_started(struct Actor *dad);
int father_want_some_sleep(struct Actor *dad);
int father_get_balance_history(struct Actor *dad);

//Operation with messages
int receive(void * self, local_id from, Message * msg);
int send(void * self, local_id dst, const Message * msg);
Message make_a_message(MessageType messageType, const char *message);
// Message make_a_message_2(MessageType messageType, const void *message, size_t payload_size);


int main(int argc, char * argv[]) {
    for (int i = 0; i < sizeof(last_recieved_message); i++) {
        last_recieved_message[i] = STOP;
    }
    
    // Check for parameters 
    int32_t children_number;
    balance_t daughter_bank_account[argc-3];
    if (strcmp(argv[1], "-p") != 0 || atoi(argv[2]) == 0 || atoi(argv[2]) != argc-3){
        // printf("Неопознанный ключ или неверное количество аргументов! Пример: -p <количество процессов>\n");
        exit(1);
    }
    else{
        if (atoi(argv[2]) > 10 || atoi(argv[2]) <= PARENT_ID){
            //printf("Некорректное количество процессов для создания (0 < x <= 9)!\n");
            exit(1);
        }   
        else {
            children_number = atoi(argv[2]);
            size_t i;
            for (i = 1; i <= children_number; i++){
                if (atoi(argv[2+i]) > 0 && atoi(argv[2+i]) < 100) 
                    daughter_bank_account[i-1] = atoi(argv[2+i]);
                else {
                    //printf("Некорректный счет для %zu дочери (1 < x <= 99)!\n", i);
                    exit(1);
                }
            }
        } 
    }
    //Try to create or open files
    int events_file = open(events_log, O_WRONLY | O_APPEND | O_CREAT, 0777);
    //int events_file = open(events_log, O_WRONLY | O_TRUNC | O_CREAT);
    if (events_file == -1)
        exit(1);

    //Make pipes
    make_a_pipes(children_number);
    
    events_f_to_pa1(events_file);
    close(events_file);

    //The appearance of the father
    struct Actor father;
    actor_dad(&father, getpid(), children_number);

    //Start of fork (born of daughters)
    struct Actor daughter;
    become_a_dad(&father, &daughter, children_number, daughter_bank_account);
    if (im_not_a_father){
        leave_needed_pipes(daughter.my_id, children_number); //Close extra pipes for child processes

        if (started_to_send(&daughter) != 0) {
            printf("Process %d couldn't send messages STARTED", daughter.my_id);
            exit(8);
        } 

        if (prepare_for_work(&daughter) != 0) //Synchronization before useful work
            exit(9);
        
        if (at_work(&daughter) != 0) //Useful work
            exit(10);

        if(done_to_send(&daughter) != 0) {
            printf("Process %d couldn't send messages DONE", daughter.my_id);
            exit(11);
        }
        
        
        if (before_a_sleep(&daughter) != 0) //Synchronization after useful work
            exit(12);
        
        if (send_balance_history(&daughter) != 0) 
            exit(13);
        
        
        close_rest_of_pipes(daughter.my_id, children_number);
        exit(0);
    }
    else{
        leave_needed_pipes(father.my_id, children_number); //Close extra pipes for main processes
        while (father_check_started(&father) != 0)
            continue;

        bank_robbery(&father, (local_id) father.my_kids);
        
        Message stopMessage = make_a_message(STOP, "");
        send_multicast(&father, &stopMessage);
        int status;
        while (children_number != 0) 
            if ((status = waitpid(-1, NULL, WEXITED || WNOHANG)) <= 0) children_number--;

        while(father_want_some_sleep(&father) != 0)
            continue;

        while (father_get_balance_history(&father) != 0)
            continue;
        
        close_rest_of_pipes(PARENT_ID, father.my_kids);
        
        close_events_file();
    } 
    return 0;
}
