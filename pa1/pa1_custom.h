/**
 * @file     pa1_custom.c
 * @author   Apykhin Artem & Grechukhin Kirill
 * @date     September, 2023
 * @brief    Constants for pa1.c
 */

#ifndef __IFMO_DISTRIBUTED_CLASS_PA1_CUSTOM__H
#define __IFMO_DISTRIBUTED_CLASS_PA1_CUSTOM__H

#include "common.h"
#include "ipc.h"
#include "pa1.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#define im_not_a_father father.my_pid != getpid()

int fd[16][16][2];
uint8_t children_number;

static const char * const pipe_closed_read_from_for =
    "Pipe closed in process %d for reading into %d process from %d process.\n";
    
static const char * const pipe_closed_write_from_into =
    "Pipe closed in process %d for writing from %d process into %d process.\n";

static const char * const pipe_opened =
    "Pipes opened for writing/reading %d.\n";

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

#endif // __IFMO_DISTRIBUTED_CLASS_PA1_CUSTOM__H
