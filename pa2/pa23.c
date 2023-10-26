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

void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount) {
    struct Actor *father = (struct Actor *)parent_data;
    
}

int main(int argc, char * argv[]) {
    for (int i = 0; i < 16; i++) {
        last_recieved_message[i] = STOP;
    }
    // Check for parameters 
    int32_t children_number;
    if (argc != 3 || strcmp(argv[1], "-p") != 0 || atoi(argv[2]) == 0){
        // printf("Неопознанный ключ или неверное количество аргументов! Пример: -p <количество процессов>\n");
        exit(1);
    }
    else{
        if (atoi(argv[2]) > 9 || atoi(argv[2]) <= PARENT_ID){
            // printf("Некорректное количество процессов для создания (0 < x <= 9)!\n");
            exit(1);
        }   
        else
            children_number = atoi(argv[2]);
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
        //TODO
        //bank_robbery(parent_data);
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
