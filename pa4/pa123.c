/**
 * @file     pa1.c
 * @author   Apykhin Artem & Grechukhin Kirill
 * @date     September, 2023
 * @brief    C library for interprocess interaction
 */

#include "common.h"
#include "ipc.h"
#include "pa1234_custom.h"
#include "pa2345.h"

static char buffer[100] = "";

static int events_f;
static timestamp_t current_logical_time = 0;

static csQueueElement csQueue[1000];
static int csQueueHead = 0;
static int csQueueBottom = 0;

csQueueElement makeElement(timestamp_t time, local_id id) {
    csQueueElement element;
    element.time = time;
    element.id = id;
    return element;
}


void addElement(csQueueElement *csQueue, csQueueElement element) {
    csQueue[csQueueHead] = element;
    csQueueHead++;
}

csQueueElement popElement(csQueueElement *csQueue) {
    csQueueElement element = csQueue[csQueueBottom];
    csQueueBottom++;
    return element;
}

timestamp_t max(timestamp_t a, timestamp_t b) {
    if (a > b) {
        return a;
    }
    return b;
}

void events_f_to_pa1(int events_file){
    dup2(events_file, events_f);
}

void close_events_file(){
    close(events_f);
}

// initialization 
void actor_dad(struct Actor *dad, pid_t zero_dad, int32_t children_number){
    dad->my_id = PARENT_ID;
    dad->my_pid = zero_dad;
    dad->my_father_pid = -1;
    dad->my_kids = children_number;
    dad->my_sisters = 0;
    // dad->my_balance.s_balance = 0;
}

void actor_daughter(struct Actor *daughter, local_id id, pid_t pid, pid_t father_pid, int32_t children_number) {
    daughter->my_id = id;
    daughter->my_pid = pid;
    daughter->my_father_pid = father_pid;
    daughter->my_kids = 0;
    daughter->my_sisters = children_number-1;
}

// pid named dad make fork kids times 
void become_a_dad(struct Actor *dad, struct Actor *daughter, int32_t children_number){
    //first_daughter
    uint8_t fork_result = fork();
    local_id id = PARENT_ID + 1;
    if (fork_result == 0) {
        actor_daughter(daughter, id, getpid(), dad->my_pid, children_number);
        return; 
    }
    
    //rest daughters
    for(uint8_t i = 1; i < dad->my_kids; i++){
        if (fork_result > 0) {
            fork_result = fork();
            id++;
        } 
        if (fork_result == 0 && getpid() != dad->my_pid){
            actor_daughter(daughter, id, getpid(), dad->my_pid, children_number);
            return;
        }
    }
}

Message make_a_message(MessageType messageType, const char *message) {
    Message msg;
    
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = strlen(message);
    msg.s_header.s_local_time = current_logical_time;
    msg.s_header.s_type = messageType;
    memset(msg.s_payload, 0, sizeof(msg.s_payload));
    strncpy(msg.s_payload, message, sizeof(msg.s_payload));
    
    return msg;
}

Message make_a_message_2(MessageType messageType, void* message, size_t payload_size) {
    Message msg;
    
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = payload_size;
    msg.s_header.s_local_time = current_logical_time;
    msg.s_header.s_type = messageType;
    memset(msg.s_payload, 0, sizeof(msg.s_payload));
    memcpy(msg.s_payload, message, sizeof(msg.s_payload));
    
    return msg;
}

int send_start(struct Actor *daughter) {
    current_logical_time++;
    Message msg = make_a_message(STARTED, buffer);
    if (send_multicast(daughter, &msg) != 0){
        printf("Write error\n");
        return 1;
    }
    return 0;
}

int prepare_for_work(struct Actor *daughter){
    Message msg = make_a_message(STARTED, buffer);
    while (1) {
        for (uint8_t i = 1; i <= daughter->my_sisters+1; i++) {
            if (daughter->my_id != i && last_recieved_message[i] != STARTED) {
                if (receive(daughter, i, &msg) != 0)
                    break;
                if (msg.s_header.s_type == STARTED) {
                    last_recieved_message[i] = STARTED;
                    current_logical_time = max(msg.s_header.s_local_time, current_logical_time)+1;
                }
            }
        }
        
        uint8_t recivied_started = 0;
        for (uint8_t i = 1; i <= (daughter->my_sisters)+1; i++)
            if (last_recieved_message[i] == STARTED && i != daughter->my_id) recivied_started++;


        if (recivied_started == daughter->my_sisters) {
            sprintf(buffer, log_received_all_started_fmt, current_logical_time, daughter->my_id);
            printf(log_received_all_started_fmt, current_logical_time, daughter->my_id);
            write(events_f, buffer, strlen(buffer)); 
            return 0;
        }
    }

    return 1;
}

void mySwap(csQueueElement *csQueue, int left, int right) {
    csQueueElement element = csQueue[left];
    csQueue[left] = csQueue[right];
    csQueue[right] = element;
}

void sortQ(csQueueElement *csQueue) {
    for (int i = csQueueBottom; i < csQueueHead; i++) {
        int wasSwap = 0;
        for (int j = i; j < csQueueHead-1; j++) {
            if (csQueue[j].time > csQueue[j+1].time) {
                mySwap(csQueue, j, j+1);
                wasSwap = 1;
            } else if (csQueue[j].time == csQueue[j+1].time) {
                if (csQueue[j].id > csQueue[j+1].id) {
                    mySwap(csQueue, j, j+1);
                    wasSwap = 1;
                }
            }
        }
        if (wasSwap == 0) {
            break;
        }
    }
}


int at_work(struct Actor *daughter){
    //Some kind of work
    uint8_t max_iteration =  daughter->my_id * 5;
    
    if (critical_area_enable) {
        int neededReleasedReplies = daughter->my_sisters;
        for (uint8_t i = 1; i <= max_iteration; i++) {
            //Send REQUEST
            current_logical_time++;
            Message requestMessage = make_a_message_2(CS_REQUEST, &daughter->my_id, sizeof(local_id));
            send_multicast(daughter, &requestMessage);

            csQueueElement element = makeElement(current_logical_time, daughter->my_id);
            addElement(csQueue, element);
            //Release OK from all
            Message someMessage;
            int releasedReplies = 0;
            while (releasedReplies < neededReleasedReplies) {
                someMessage = make_a_message(ACK, "");
                receive_any(daughter, &someMessage);
                if (someMessage.s_header.s_type == CS_REPLY) {
                    current_logical_time = max(current_logical_time, someMessage.s_header.s_local_time) + 1;
                    releasedReplies++;
                }
                if (someMessage.s_header.s_type == CS_REQUEST) {
                    current_logical_time = max(current_logical_time, someMessage.s_header.s_local_time) + 1;
                    local_id receivier_ID;
                    memcpy(&receivier_ID, someMessage.s_payload, sizeof(someMessage.s_payload));
                    csQueueElement element = makeElement(someMessage.s_header.s_local_time, receivier_ID);
                    addElement(csQueue, element);
                    someMessage = make_a_message(CS_REPLY, "");
                    send(daughter, receivier_ID, &someMessage);
                }
                if (someMessage.s_header.s_type == CS_RELEASE) {
                    current_logical_time = max(current_logical_time, someMessage.s_header.s_local_time) + 1;
                    sortQ(csQueue);
                    popElement(csQueue);
                }
                if (someMessage.s_header.s_type == DONE) {
                    local_id receivier_ID;
                    memcpy(&receivier_ID, someMessage.s_payload, sizeof(someMessage.s_payload));
                    last_recieved_message[receivier_ID] = DONE;
                    neededReleasedReplies = neededReleasedReplies - 1;
                }
            }
            //Sort Q
            sortQ(csQueue);
            
            while (csQueue[csQueueBottom].id != daughter->my_id) {
                Message releaseMessageFrom;
                while (releaseMessageFrom.s_header.s_type != CS_RELEASE) {
                    receive(daughter, csQueue[csQueueBottom].id, &releaseMessageFrom);
                }
                releaseMessageFrom = make_a_message(ACK, "");
                popElement(csQueue);
            }
            
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, log_loop_operation_fmt, daughter->my_id, i, max_iteration);
            print(buffer);
            write(events_f, buffer, strlen(buffer));
            
            //Release CS
            popElement(csQueue);
            Message releaseMessage = make_a_message_2(CS_RELEASE, &daughter->my_id, sizeof(local_id));
            send_multicast(daughter, &releaseMessage);
        }
    } else {
        for (uint8_t i = 1; i <= max_iteration; i++) {
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, log_loop_operation_fmt, daughter->my_id, i, max_iteration);
            print(buffer);
            write(events_f, buffer, strlen(buffer));
        }
    }
    return 0;
}

int send_done(struct Actor *daughter) {
    current_logical_time++;
    Message done_message = make_a_message_2(DONE, &daughter->my_id, sizeof(local_id));
    if (send_multicast(daughter, &done_message) != 0) {
        printf("Write error\n");
        return 1;
    }
    return 0;
}


int before_a_sleep(struct Actor *daughter){
    Message done_message = make_a_message(ACK, "");
    while (1) {
        for (uint8_t i = 1; i <= daughter->my_sisters+1; i++) {
            if (daughter->my_id != i && last_recieved_message[i] != DONE) {
                if (receive(daughter, i, &done_message) != 0)
                    break;
                if (done_message.s_header.s_type == DONE) {
                    last_recieved_message[i] = DONE;
                    current_logical_time = max(current_logical_time, done_message.s_header.s_local_time) + 1;
                }
            }
        }
        
        uint8_t recivied_done = 0;
        for (uint8_t i = 1; i <= (daughter->my_sisters)+1; i++)
            if (last_recieved_message[i] == DONE && i != daughter->my_id) recivied_done++;
        
            
        if (recivied_done == daughter->my_sisters) {
            sprintf(buffer, log_received_all_done_fmt, current_logical_time, daughter->my_id);
            printf(log_received_all_done_fmt, current_logical_time, daughter->my_id);
            write(events_f, buffer, strlen(buffer));
            return 0;
        }
    }
    
    return 1;
}

int father_check_started(struct Actor *dad) {
    current_logical_time++;
    Message msg = make_a_message(STARTED, buffer);
    while (1) {
        uint8_t recievers = dad->my_kids;
        for (uint8_t i = 1; i <= recievers; i++) {
            if (dad->my_id != i && last_recieved_message[i] != STARTED) {
                if (receive(dad, i, &msg) != 0)
                    break;
                if (msg.s_header.s_type == STARTED) {
                    last_recieved_message[i] = STARTED;
                    current_logical_time = max(current_logical_time, msg.s_header.s_local_time) + 1;
                }
            }
        }
        
        uint8_t recivied_started = 0;
        for (uint8_t i = 1; i <= (dad->my_kids); i++)
            if (last_recieved_message[i] == STARTED && i != PARENT_ID) recivied_started++;
            
        if (recivied_started == dad->my_kids)
            return 0;
    }
    
    return 1;
}

int father_want_some_sleep(struct Actor *dad) {
    current_logical_time++;
    Message done_message = make_a_message(DONE, buffer);
    while (1) {
        for (uint8_t i = 1; i <= dad->my_kids; i++) {
            if (dad->my_id != i && last_recieved_message[i] != DONE) {
                if (receive(dad, i, &done_message) != 0)
                    break;
                if (done_message.s_header.s_type == DONE) {
                    last_recieved_message[i] = DONE;
                    current_logical_time = max(current_logical_time, done_message.s_header.s_local_time) + 1;
                }
            }
        }
        
        uint8_t recivied_done = 0;
        for (uint8_t i = 1; i <= (dad->my_kids); i++)
            if (last_recieved_message[i] == DONE && i != PARENT_ID) recivied_done++;
        
        if (recivied_done == dad->my_kids)
            return 0;
        }
    
    return 1;
}
