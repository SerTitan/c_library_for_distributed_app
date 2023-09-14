#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include <stdio.h>

//Файлы логово и сообщений для пайпов
static const char * const events_log;
static const char * const pipes_log;

int main() { 
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
    return 0;
}
