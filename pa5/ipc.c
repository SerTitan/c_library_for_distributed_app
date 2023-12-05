/**
 * @file     sro.c
 * @author   Apykhin Artem & Grechukhin Kirill
 * @date     October, 2023
 * @brief    Send/Recieve operations
 */

#include "common.h"
#include "ipc.h"
#include "pa2345.h"
#include "pa12345_custom.h"

static int fd[12][12][2];
static int is_closed[12][12] = {{0}}; //Number in cell i,j describe status of pipe between i and j. 0 - it's opened. 1 - it's closed for reading. 2 - it's closed for writing. 3 - it's completley closed;

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

void make_a_pipes(int32_t children_number) {
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
                num_of_pipes++;
            }
        }
    }
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
                    }
                    if (j != PARENT_ID) {
                        if (close(fd[i][j][0]))
                            exit(14);
                        is_closed[i][j] += 1;
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
                    }
                    if (i != id) {
                        if (close(fd[i][j][1]))
                            exit(16);
                        is_closed[i][j] += 2;
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
                }
                if (is_closed[i][j] == 1 || is_closed[i][j] == 0) {
                    if (close(fd[i][j][1])) 
                        exit(18);
                    is_closed[i][j] += 2;
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
    if (receiver->my_id == from)
        return -1;
    int read_fd = fd[from][receiver->my_id][0];
    if (read(read_fd, &msg_hdr, sizeof(MessageHeader)) < (int)sizeof(MessageHeader))
        return -1;
    msg->s_header = msg_hdr;
    if (read(read_fd, msg->s_payload, msg->s_header.s_payload_len) < msg->s_header.s_payload_len) 
        return -1;
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
            if (receive(receiver, i, msg) == 0) {
                return 0;
            }
        }
    }
    return 1;
}
