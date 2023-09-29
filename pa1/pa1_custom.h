/**
 * @file     pa1_custom.c
 * @Author   Apykhin Artem & Grechukhin Kirill
 * @date     September, 2023
 * @brief    Constants for pa1.c
 */

#ifndef __IFMO_DISTRIBUTED_CLASS_PA1_CUSTOM__H
#define __IFMO_DISTRIBUTED_CLASS_PA1_CUSTOM__H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#define im_not_a_father father.my_pid != getpid()

static const char * const pipe_closed_read_from_for =
    "Pipe closed in process %d for reading into %d process from %d process.\n";
    
static const char * const pipe_closed_write_from_into =
    "Pipe closed in process %d for writing from %d process into %d process.\n";

static const char * const pipe_opened =
    "Pipes opened for writing/reading %d.\n";

#endif // __IFMO_DISTRIBUTED_CLASS_PA1_CUSTOM__H