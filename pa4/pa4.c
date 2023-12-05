/**
 * @file     pa23.c
 * @author   Apykhin Artem & Grechukhin Kirill
 * @date     November, 2023
 * @brief    C library for interprocess interaction
 */

#include "common.h"
#include "ipc.h"
#include "pa1234_custom.h"
#include "pa2345.h"

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
void actor_daughter(struct Actor *daughter, local_id id, pid_t pid, pid_t father_pid, int32_t children_number);
void become_a_dad(struct Actor *dad, struct Actor *daughter, int32_t children_number);

//Stages of daughter's working process
int send_start(struct Actor *daughter);
int prepare_for_work(struct Actor *daughter);
int at_work(struct Actor *daughter);
// int send_done();
int send_done(struct Actor *daughter);
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
    int32_t children_number;
    uint8_t argv_num;


    for (int i = 0; i < sizeof(last_recieved_message); i++) {
        last_recieved_message[i] = STOP;
    }
    
    if (strcmp(argv[1], "--mutexl") != 0) {
        critical_area_enable = false;
        argv_num = 1;
    }
    else {
        critical_area_enable = true;
        argv_num = 2;
    }
    
    if (strcmp(argv[argv_num], "-p") != 0 || atoi(argv[argv_num + 1]) == 0){
        // printf("Неопознанный ключ или неверное количество аргументов! Пример: -p <количество процессов>\n");
        exit(1);
    }
    else {
        if (atoi(argv[argv_num+1]) > 10 || atoi(argv[argv_num+1]) <= PARENT_ID){
            //printf("Некорректное количество процессов для создания (0 < x <= 9)!\n");
            exit(2);
        }  
        else
            children_number = atoi(argv[argv_num+1]);
    }

    //Try to create or open files
    int events_file = open(events_log, O_WRONLY | O_APPEND | O_CREAT, 0777);
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
    become_a_dad(&father, &daughter, children_number);
    if (im_not_a_father){
        leave_needed_pipes(daughter.my_id, children_number); //Close extra pipes for child processes

        if (send_start(&daughter) != 0)
            exit(9);

        if (prepare_for_work(&daughter) != 0) //Synchronization before useful work
            exit (10);
        
        if (at_work(&daughter) != 0) //Useful work
            exit(11);
        
        if (send_done(&daughter) != 0)
            exit(12);
        
        if (before_a_sleep(&daughter) != 0) //Synchronization after useful work
            exit(13);
        
        close_rest_of_pipes(daughter.my_id, children_number);
        exit(0);
    }
    else{
        leave_needed_pipes(father.my_id, children_number); //Close extra pipes for main processes
        int status;
        if (father_check_started(&father) != 0)
            exit(15);
    
        while (children_number != 0) 
            if ((status = waitpid(-1, NULL, WEXITED || WNOHANG)) <= 0) children_number--;

        if(father_want_some_sleep(&father) != 0)
            exit(16);

        close_rest_of_pipes(PARENT_ID, father.my_kids);
        
        close_events_file();
    } 
    return 0;
}
