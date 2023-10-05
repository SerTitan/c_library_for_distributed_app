/**
 * @file     sro.c
 * @author   Apykhin Artem & Grechukhin Kirill
 * @date     October, 2023
 * @brief    Send/Recieve operations
 */

#include "pa1_custom.h"

int send_multicast(void * self, const Message * msg) {
    struct Actor *sender = (struct Actor *)self;
    for (uint8_t i = 0; i <= children_number; i++) {
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