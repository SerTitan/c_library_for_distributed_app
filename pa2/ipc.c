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
#include <fcntl.h>


static int fd[12][12][2];
static int is_closed[12][12] = {{0}}; //Number in cell i,j describe status of pipe between i and j. 0 - it's opened. 1 - it's closed for reading. 2 - it's closed for writing. 3 - it's completley closed;
static char buff[65];

static int pipes_f;

void close_pipe_file(){
    close(pipes_f);
}

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}

void make_a_pipes(int32_t children_number, int pipes_file){
    dup2(pipes_file, pipes_f);
    int num_of_pipes = 0;
    for (int32_t i = 0; i <= children_number; i++) {
        for (int32_t j = 0; j <= children_number; j++) {
            if (i != j) {
                if (pipe(fd[i][j]) == -1) {
                    perror("pipe");
                    exit(10);
                }
                if (set_nonblocking(fd[i][j][1]) != 0) {
                    perror("fcntl");
                    exit(11);
                }
                if (set_nonblocking(fd[i][j][0]) != 0) {
                    perror("fcntl");
                    exit(12);
                }
                // int flags = fcntl(fd[i][j][1], F_GETFL, 0);
                // if (flags == -1) {
                //     perror("fcntl");
                //     exit(1);
                // }

                // flags |= O_NONBLOCK;
                // if (fcntl(fd[i][j][1], F_SETFL, flags) == -1) {
                //     perror("fcntl");
                //     exit(1);
                // }
                // flags = fcntl(fd[j][i][0], F_GETFL, 0);
                // if (flags == -1) {
                //     perror("fcntl");
                //     exit(1);
                // }

                // flags |= O_NONBLOCK;
                // if (fcntl(fd[j][i][0], F_SETFL, flags) == -1) {
                //     perror("fcntl");
                //     exit(1);
                // }
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
                    if (i != PARENT_ID) {               
                        if (close(fd[i][j][1]) != 0)
                            exit(13);
                        is_closed[i][j] += 2;
                        sprintf(buff, pipe_closed_write_from_into, id, i, j);
                        write(pipes_f, buff, strlen(buff));
                    }
                    if (j != PARENT_ID) {
                        if (close(fd[i][j][0]))
                            exit(14);
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
                    if (j != id) {
                        if (close(fd[i][j][0]) != 0)
                            exit(15);
                        is_closed[i][j] += 1;
                        sprintf(buff, pipe_closed_read_from_for, id, i, j);
                        write(pipes_f, buff, strlen(buff));
                    }
                    if (i != id) {
                        if (close(fd[i][j][1]))
                            exit(16);
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
                if (is_closed[i][j] == 0 || is_closed[i][j] == 2) {
                    if (close(fd[i][j][0]) != 0) 
                        exit(17);
                    is_closed[i][j] += 1;
                    sprintf(buff, pipe_closed_read_from_for, id, i, j);
                    write(pipes_f, buff, strlen(buff));
                }
                if (is_closed[i][j] == 1 || is_closed[i][j] == 0) {
                    if (close(fd[i][j][1])) 
                        exit(18);
                    is_closed[i][j] += 2;
                    sprintf(buff, pipe_closed_write_from_into, id, i, j);
                    write(pipes_f, buff, strlen(buff));   
                }      
            }
        }
    }
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

int send_multicast(void * self, const Message * msg) {
    struct Actor *sender = (struct Actor *)self;
    int32_t senders = sender->my_sisters+1;
    if (sender->my_kids != 0)
        senders = sender->my_kids;
    for (int32_t i = 0; i <= senders; i++) {
        if (sender->my_id != i) {
            int write_fd = fd[sender->my_id][i][1];
            if (write(write_fd, msg, sizeof(MessageHeader) + (msg->s_header.s_payload_len)) == -1) {
                perror("write");
                return -1;
            }
        } 
    }
    return 0;
}


int receive(void * self, local_id from, Message * msg) {
    struct Actor *receiver = (struct Actor *)self;
    MessageHeader msg_hdr;
    if (receiver->my_id == from) {
        // perror("read");
        return -1;
    }
    int read_fd = fd[from][receiver->my_id][0];
    if (read(read_fd, &msg_hdr, sizeof(MessageHeader)) < (int)sizeof(MessageHeader)) {
        // perror("read");
        return -1;
    }
    msg->s_header = msg_hdr;
    // last_recieved_message[from] = msg->s_header.s_type;
    if (read(read_fd, msg->s_payload, msg->s_header.s_payload_len) < msg->s_header.s_payload_len) {
        // perror("read tail");
        return -1;
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
            // int read_fd = fd[i][receiver->my_id][0];
            // printf("Process %d trying to read\n", receiver->my_id);
            if (receive(receiver, i, msg) == 0) {
                return 0;
            }
        }
    }
    return 1;
}


//Tests
/*
int test_send(void * self, local_id dst, int* number) {
    struct Actor *sender = (struct Actor *)self;
    int write_fd = fd[sender->my_id][dst][1];
    if (write(write_fd, number, sizeof(int)) == -1) {
            perror("write");
            return -1;
    }
    return 0;
}

int test_receive(void * self, local_id from, int* number) {
    struct Actor *receiver = (struct Actor *)self;
    int32_t recievers;
    if (receiver->my_kids == 0)
        recievers = receiver->my_sisters+1;
    else
        recievers = receiver->my_kids;
    int read_fd = fd[from][receiver->my_id][0];
            if (read(read_fd, number, sizeof(int)) == -1) {
                // printf("Read error\n");
                perror("read");
                return -1;
            }
            // printf("Process %d. Read succed\n", receiver->my_id);
    return 0;
}
*/
