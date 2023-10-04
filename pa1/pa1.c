/**
 * @file     pa1.c
 * @Author   Apykhin Artem & Grechukhin Kirill
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

static char buffer[50], buffer2[50]; 
static int fd[16][16][2];
static uint8_t children_number;
static MessageType last_recieved_message[16];
static int is_closed[16][16] = {0}; //Number in cell i,j describe status of pipe between i and j. 0 - it's opened. 1 - it's closed for reading. 2 - it's closed for writing. 3 - it's completley closed;

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
void actor_dad(struct Actor *dad, pid_t zero_dad){
    dad->my_id = PARENT_ID;
    dad->my_pid = zero_dad;
    dad->my_father_pid = -1;
    dad->my_role = DAD;
    dad->my_kids= children_number;
}

void actor_daughter(struct Actor *daughter, local_id id, pid_t pid, pid_t father_pid) {
    daughter->my_id = id;
    daughter->my_pid = pid;
    daughter->my_father_pid = father_pid;
    daughter->my_role = CHILD;
    daughter->my_kids = 0;
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

Message make_a_message(MessageType messageType, const char *message) {
    Message msg;

    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = strlen(message);
    
    switch (messageType) {
        case STARTED:
            msg.s_header.s_type = STARTED;
            break;
        case DONE:
            msg.s_header.s_type = DONE;
            break;
        default:
            msg.s_header.s_type = ACK; 
            break;
    }

    strncpy(msg.s_payload, message, sizeof(msg.s_payload));
    
    return msg;
}



int send_multicast(void * self, const Message * msg) {
    struct Actor *sender = (struct Actor *)self;
    for (uint8_t i = 0; i <= children_number; i++) {
        if (sender->my_id != i) {
            int write_fd = fd[sender->my_id][i][1];
            if (write(write_fd, msg, sizeof(Message)) == -1) {
                perror("write");
                return -1;
            }
        } 
    }
    return 0;
}


int receive_any(void * self, Message * msg){
    // struct Actor *receiver = (struct Actor *)self;
    // for (uint8_t i = 0; i <= children_number; i++) {
    //     if (receiver->my_id != i) {
    //         int read_fd = fd[receiver->my_id][i][0];
    //         if (read(read_fd, msg, sizeof(Message)) == -1) {
    //             perror("read");
    //             return -1;
    //         }
    //         last_recieved_message[i] = msg->s_header.s_type;
    //     }
    // }
    return 0;
}


int send(void * self, local_id dst, const Message * msg) {
    struct Actor *sender = (struct Actor *)self;
    int write_fd = fd[sender->my_id][dst][1];
    if (write(write_fd, msg, sizeof(Message)) == -1) {
            perror("write");
            return -1;
    }
    return 0;
}


int receive(void * self, local_id from, Message * msg) {
    //read(fd[from][0], );
    return 0;
}


//CLose unused pipes
void leave_needed_pipes(local_id id, int* f2) {
    if (id == PARENT_ID) {
        for (int8_t i = 0; i <= children_number; i++) {
            for (int8_t j = 0; j <= children_number; j++) {
                if (i != j) {
                    close(fd[i][j][1]);
                    is_closed[i][j] += 2;
                    sprintf(buffer2, pipe_closed_write_from_into, id, i, j);
                    write(*f2, buffer2, strlen(buffer2));

                    if (i != PARENT_ID) {
                        close(fd[i][j][0]);
                        is_closed[i][j] += 1;
                        sprintf(buffer2, pipe_closed_read_from_for, id, i, j);
                        write(*f2, buffer2, strlen(buffer2));
                    }
                }
            }
        }
    }
    else {
        for (int8_t i = 0; i <= children_number; i++) {
            for (int8_t j = 0; j <= children_number; j++) {
                if (i != j) {
                    if (j == PARENT_ID) {
                        close(fd[i][j][1]);
                        is_closed[i][j] += 1;
                        sprintf(buffer2, pipe_closed_read_from_for, id, i, j);
                        write(*f2, buffer2, strlen(buffer2));
                    }
                    else if (i != id) {
                        close(fd[i][j][0]);
                        is_closed[i][j] += 1;
                        sprintf(buffer2, pipe_closed_read_from_for, id, i, j);
                        write(*f2, buffer2, strlen(buffer2));
                        close(fd[i][j][1]);
                        is_closed[i][j] += 2;
                        sprintf(buffer2, pipe_closed_write_from_into, id, i, j);
                        write(*f2, buffer2, strlen(buffer2));
                    }
                }
            }
        }
    }
}


int prepare_for_work(struct Actor *dad, struct Actor *daughter){
    sprintf(buffer, log_received_all_started_fmt, daughter->my_id);
    Message msg = make_a_message(STARTED, buffer);
    if (send_multicast(&daughter, &msg) != children_number)
        return 1;
    while (1) {
        if (receive_any(&daughter, &msg) == children_number - 1) {
            break;
        }
    }
    return 0;
}


int at_work(struct Actor *dad, struct Actor *daughter){
    //Some kind of work
    sprintf(buffer, log_done_fmt, daughter->my_id);
    return 0;
}


//
int before_a_sleep(struct Actor *dad, struct Actor *daughter){
    Message done_message = make_a_message(DONE, "I'm done, dude");
    if (send_multicast(daughter, &done_message) != 0) {
        return 1;
    }
    // int received_all_done = 0;
    // Message msg;
    // while (received_all_done == 0) {
    //     received_all_done = 1;
    //     if (receive_any(daughter, &msg) != 0) {
    //         return 1;
    //     }
    //     for (int i = 0; i <= children_number; i++) {
    //         if (last_recieved_message[i] != DONE) {
    //             received_all_done = 0;
    //             break;
    //         }
    //     }
    // }   
    // sprintf(buffer, log_received_all_done_fmt, daughter->my_id);
    return 0;
}


int main(int argc, char *argv[]) {

    // Check for parameters 
    children_number = 0;
    if (argc != 3 || strcmp(argv[1], "-p") != 0 || atoi(argv[2]) == 0){
        //printf("Неопознанный ключ или неверное количество аргументов! Пример: -p <количество процессов>\n");
        exit(1);
    }
    else{
        if (atoi(argv[2]) > MAX_PROCESS_ID || atoi(argv[2]) <= PARENT_ID){
            //printf("Некорректное количество процессов для создания (0 < x <= 15)!\n");
            exit(1);
        }   
        else
            children_number = atoi(argv[2]);
    }

    //Try to create or open files
    int f1 = open(events_log, O_WRONLY | O_TRUNC | O_CREAT);
    int f2 = open(pipes_log, O_WRONLY | O_TRUNC | O_CREAT); 
    if (f1 == -1)
        exit(1);
    if (f2 == -1)
        exit(1);

    //Make pipes
    int num_of_pipes = 0;
    for (int i = 0; i <= children_number; i++) {
        for (int j = 0; j <= children_number; j++) {
            if (i != j) {
                if (pipe(fd[i][j]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            num_of_pipes++;
            }
        }
    }
    
    sprintf(buffer2, pipe_opened, num_of_pipes);
    write(f2, buffer2, strlen(buffer2));

    //The appearance of the father
    struct Actor father;
    actor_dad(&father, getpid());

    //Start of fork (born of daughters)
    struct Actor daughter;
    become_a_dad(&father, &daughter);

    if (im_not_a_father){
        // leave_needed_pipes(daughter.my_id, &f2); //Close extra pipes for child processes

        write(f1, buffer, strlen(buffer));
        if (prepare_for_work(&father, &daughter) == 0) { //Synchronization before useful work
            printf("Пока не пишем.");
            write(f1, buffer, strlen(buffer)); 
        }
        else
            exit(1);

        if (at_work(&father, &daughter) == 0) //Useful work
            write(f1, buffer, strlen(buffer));
        else
            exit(1);
        //Check fd    
        // for (int i = 0; i <= children_number; i++) {
        //     printf("%d ", daughter.my_id);
        //     for (int j = 0; j <= children_number; j++) {
        //     printf("%3d ", is_closed[i][j]); // Используйте %2d для выравнивания чисел по правому краю
        // }
        // printf("\n"); // Переход на следующую строку после каждой строки массива
        // }
        //End of check fd


        if (before_a_sleep(&father, &daughter) == 0) //Synchronization after useful work
            write(f1, buffer, strlen(buffer));
        else
            exit(1);

        //Check read
        Message msg = make_a_message(STARTED, "Are you gay?");
        if (daughter.my_id != children_number) {
            send(&daughter, daughter.my_id + 1, &msg);
        }
        
        if (daughter.my_id != 1) {
            int read_fd = fd[daughter.my_id][daughter.my_id-1][0];
                if (read(read_fd, &msg, sizeof(Message)) == -1) {
                    perror("read");
                    printf("Process %d Can't read\n", daughter.my_id);
                } else {
                    printf("Process %d Can read\n", daughter.my_id);
                    printf("%d", msg.s_header.s_magic);
                }
                last_recieved_message[daughter.my_id-1] = msg.s_header.s_type;
            //End of check read
        }
        exit(0);
    }
    else{
        leave_needed_pipes(father.my_id, &f2); //Close extra pipes for main processes
        int status;
         //Check fd    
        // for (int i = 0; i <= children_number; i++) {
        //     printf("%d ", father.my_id);
        //     for (int j = 0; j <= children_number; j++) {
        //     printf("%3d ", is_closed[i][j]); // Используйте %2d для выравнивания чисел по правому краю
        // }
        // printf("\n"); // Переход на следующую строку после каждой строки массива
        // }
        //End of check fd
        while (1)
            if (wait(&status) > 0) break; //Waiting for the end of child processes 
        //Closing of files
        close(f1);
        close(f2);
    } 
    return 0;
}
