/**
 * @file     sro.c
 * @author   Apykhin Artem & Grechukhin Kirill
 * @date     October, 2023
 * @brief    Send/Recieve operations
 */

#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include "pa1_custom.h"

int fd[12][12][2];
static int is_closed[12][12] = {{0}}; //Number in cell i,j describe status of pipe between i and j. 0 - it's opened. 1 - it's closed for reading. 2 - it's closed for writing. 3 - it's completley closed;
char buff[65];

static int pipes_f;

void close_pipe_file(){
    close(pipes_f);
}

void make_a_pipes(int32_t children_number, int pipes_file){
    dup2(pipes_file, pipes_f);
    int num_of_pipes = 0;
    for (int32_t i = 0; i <= children_number; i++) {
        for (int32_t j = 0; j <= children_number; j++) {
            if (i != j) {
                if (pipe(fd[i][j]) == -1) {
                    perror("pipe");
                    exit(1);
                }
                num_of_pipes++;
            }
        }
    }
    sprintf(buff, pipe_opened, num_of_pipes);
    write(pipes_f, buff, strlen(buff));
}

//CLose unused pipes
void leave_needed_pipes(local_id id, int32_t children_number) {
    if (id == PARENT_ID) {
        for (int32_t i = 0; i <= children_number; i++) {
            for (int32_t j = 0; j <= children_number; j++) {
                if (i != j) {
                    if (close(fd[i][j][1]) != 0) {
                        //printf("Can't close pipe between %d and %d for writing in process %d", i, j, id);
                        exit(1);
                    }
                    is_closed[i][j] += 2;
                    sprintf(buff, pipe_closed_write_from_into, id, i, j);
                    write(pipes_f, buff, strlen(buff));
                
                    if (i != PARENT_ID) {
                        if (close(fd[i][j][0])) {
                            //printf("Can't close pipe between %d and %d for reading in process %d", i, j, id);
                            exit(1);
                        }
                        is_closed[i][j] += 1;
                        sprintf(buff, pipe_closed_read_from_for, id, i, j);
                        write(pipes_f, buff, strlen(buff));
                    }
                }
            }
        }
    }
    else {
        for (int32_t i = 0; i <= children_number; i++) {
            for (int32_t j = 0; j <= children_number; j++) {
                if (i != j) {
                    if (i != id) {
                        if (close(fd[i][j][0]) != 0) {
                            //printf("Can't close pipe between %d and %d for reading in process %d", i, j, id);
                            exit(1);
                        }
                        
                        is_closed[i][j] += 1;
                        sprintf(buff, pipe_closed_read_from_for, id, i, j);
                        write(pipes_f, buff, strlen(buff));
                        if (close(fd[i][j][1])) {
                            //printf("Can't close pipe between %d and %d for writing in process %d", i, j, id);
                            exit(1);
                        }
                        is_closed[i][j] += 2;
                        sprintf(buff, pipe_closed_write_from_into, id, i, j);
                        write(pipes_f, buff, strlen(buff));
                    }
                }
            }
        }
    }
}

void close_rest_of_pipes(local_id id, int32_t children_number) {
    for (int32_t i = 0; i <= children_number; i++) {
        for (int32_t j = 0; j <= children_number; j++) {
            if (i != j) {
                if (i == id) {
                    if (close(fd[i][j][0]) != 0) {
                            //printf("Can't close pipe between %d and %d for reading in process %d", i, j, id);
                            exit(1);
                    }
                    is_closed[i][j] += 1;
                    sprintf(buff, pipe_closed_read_from_for, id, i, j);
                    write(pipes_f, buff, strlen(buff));
                    if (close(fd[i][j][1])) {
                            //printf("Can't close pipe between %d and %d for writing in process %d", i, j, id);
                            exit(1);
                    }
                    is_closed[i][j] += 2;
                    sprintf(buff, pipe_closed_write_from_into, id, i, j);
                    write(pipes_f, buff, strlen(buff));         
                }
            }
        }
    }
}

int send_multicast(void * self, const Message * msg) {
    struct Actor *sender = (struct Actor *)self;
    //printf("%d\n", sender->my_sisters);
    for (int32_t i = 0; i <= sender->my_sisters+1; i++) {
        if (sender->my_id != i) {
            int write_fd = fd[sender->my_id][i][1];
            // printf("Try to write\n");
            if (write(write_fd, msg, sizeof(MessageHeader) + (msg->s_header.s_payload_len)) == -1) {
                perror("write");
                return -1;
            }
            else
                sprintf(buff, write_to_pipe, sender->my_id, i);
        } 
    }
    return 0;
}


int receive_any(void * self, Message * msg){
    struct Actor *receiver = (struct Actor *)self;
    int32_t recievers;
    if (receiver->my_kids == 0)
        recievers = receiver->my_sisters+1;
    else
        recievers = receiver->my_kids;
    for (int32_t i = 0; i <= recievers; i++) {
        if (receiver->my_id != i) {
            int read_fd = fd[receiver->my_id][i][0];
            if (read(read_fd, msg, sizeof(Message)) == -1) {
                perror("read");
                return -1;
            }
            last_recieved_message[i] = msg->s_header.s_type;
        }
    }
    return 0;
}


int send(void * self, local_id dst, const Message * msg) {
    struct Actor *sender = (struct Actor *)self;
    int write_fd = fd[sender->my_id][dst][1];
    if (write(write_fd, msg, sizeof(MessageHeader) + (msg->s_header.s_payload_len)) == -1) {
            perror("write");
            return -1;
    }
    return 0;
}


int receive(void * self, local_id from, Message * msg) {
    //read(fd[from][0], );
    return 0;
}
