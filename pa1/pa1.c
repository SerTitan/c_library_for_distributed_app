#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>

//Файлы логово и сообщений для пайпов
static const char * const events_log;
static const char * const pipes_log;

int main(int argc, char *argv[]) { 
    int32_t proc_number = 0;
    if (argc != 3 || strcmp(argv[1], "-p") != 0 || atoi(argv[2]) == 0){
        exit(1);
    }
    else{
        proc_number = atoi(argv[2]);
        // int num_children = atoi(argv[2]);
        if (proc_number > 10 || proc_number < 0)
            exit(1);
    }
    FILE *events_file = fopen(events_log, "r");
    FILE *pipes_file = fopen(pipes_log, "r"); 
    if (!events_file) {
         events_file = fopen(events_log, "w");
        if (events_file)
            fclose(events_file);
        else
            return 1;
    }
    if (!pipes_file) {
         pipes_file = fopen(pipes_log, "w");
        if (pipes_file)
            fclose(pipes_file);
        else
            return 1;
    }

    //pid_t child_pid = fork();

    printf("%d\n" , proc_number);
    
    return 0;
}
