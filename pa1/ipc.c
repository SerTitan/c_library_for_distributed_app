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
char* buff[65];

extern int *pipes_file;

void make_a_pipes(uint8_t children_number){
    int num_of_pipes = 0;
    for (int i = 0; i <= children_number; i++) {
        for (int j = 0; j <= children_number; j++) {
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
    write(pipes_file, buff, strlen(buff));
}

//CLose unused pipes
void leave_needed_pipes(local_id id, uint8_t children_number) {
    if (id == PARENT_ID) {
        for (int8_t i = 0; i <= children_number; i++) {
            for (int8_t j = 0; j <= children_number; j++) {
                if (i != j) {
                    close(fd[i][j][1]);
                    is_closed[i][j] += 2;
                    sprintf(buff, pipe_closed_write_from_into, id, i, j);
                    write(pipes_file, buff, strlen(buff));
                
                    if (i != PARENT_ID) {
                        close(fd[i][j][0]);
                        is_closed[i][j] += 1;
                        sprintf(buff, pipe_closed_read_from_for, id, i, j);
                        write(pipes_file, buff, strlen(buff));
                    }
                }
            }
        }
    }
    else {
        for (int8_t i = 0; i <= children_number; i++) {
            for (int8_t j = 0; j <= children_number; j++) {
                if (i != j) {
                    if (i != id) {
                        close(fd[i][j][0]);
                        is_closed[i][j] += 1;
                        sprintf(buff, pipe_closed_read_from_for, id, i, j);
                        write(pipes_file, buff, strlen(buff));
                        close(fd[i][j][1]);
                        is_closed[i][j] += 2;
                        sprintf(buff, pipe_closed_write_from_into, id, i, j);
                        write(pipes_file, buff, strlen(buff));
                    }
                }
            }
        }
    }
}

void close_rest_of_pipes(local_id id, uint8_t children_number) {
    for (int8_t i = 0; i <= children_number; i++) {
        for (int8_t j = 0; j <= children_number; j++) {
            if (i != j) {
                if (i == id) {
                    close(fd[i][j][0]);
                    is_closed[i][j] += 1;
                    sprintf(buff, pipe_closed_read_from_for, id, i, j);
                    write(pipes_file, buff, strlen(buff));
                    close(fd[i][j][1]);
                    is_closed[i][j] += 2;
                    sprintf(buff, pipe_closed_write_from_into, id, i, j);
                    write(pipes_file, buff, strlen(buff));         
                }
            }
        }
    }
}

int send_multicast(void * self, const Message * msg) {
    struct Actor *sender = (struct Actor *)self;
    // printf("Starting multisend\n");
    for (uint8_t i = 0; i <= sender->my_sisters+1; i++) {
        if (sender->my_id != i) {
            int write_fd = fd[sender->my_id][i][1];
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
    for (uint8_t i = 0; i <= children_number; i++) {
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
