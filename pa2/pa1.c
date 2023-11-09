/**
 * @file     pa1.c
 * @author   Apykhin Artem & Grechukhin Kirill
 * @date     September, 2023
 * @brief    C library for interprocess interaction
 */

#include "common.h"
#include "ipc.h"
#include "pa1_custom.h"
#include "pa2345.h"
#include "banking.h"

static char buffer[100] = "";

static int events_f;

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
    dad->my_balance.s_balance = 0;
}

void actor_daughter(struct Actor *daughter, local_id id, pid_t pid, pid_t father_pid, int32_t children_number, balance_t balance) {
    daughter->my_id = id;
    daughter->my_pid = pid;
    daughter->my_father_pid = father_pid;
    daughter->my_kids = 0;
    daughter->my_sisters = children_number-1;
    daughter->my_balance.s_balance = balance;
<<<<<<< HEAD
    daughter->my_balance.s_time = 0;
    daughter->my_balance.s_balance_pending_in = 0;
=======
>>>>>>> db2b80ce86e764242e52164a15e99bba57f5d8ea
    daughter->history.s_id = id;
    daughter->history.s_history[0] = daughter->my_balance;
    BalanceState nullBalance;
    nullBalance.s_balance = -1;
    nullBalance.s_time = -1;
<<<<<<< HEAD
    nullBalance.s_balance_pending_in = 0;
=======
>>>>>>> db2b80ce86e764242e52164a15e99bba57f5d8ea
    for (int i = 1; i < MAX_T; i++) {
        daughter->history.s_history[i] = nullBalance;
    }
    sprintf(buffer, log_started_fmt, get_physical_time(), id, pid, father_pid, balance);
    printf(log_started_fmt, get_physical_time(), id, pid, father_pid, balance);
}

// pid named dad make fork kids times 
void become_a_dad(struct Actor *dad, struct Actor *daughter, int32_t children_number, balance_t* daughter_bank_account){
    //first_daughter
    uint8_t fork_result = fork();
    local_id id = PARENT_ID + 1;
    if (fork_result == 0) {
        actor_daughter(daughter, id, getpid(), dad->my_pid, children_number, daughter_bank_account[0]);
        return; 
    }
    
    //rest daughters
    for(uint8_t i = 1; i < dad->my_kids; i++){
        if (fork_result > 0) {
            fork_result = fork();
            id++;
        } 
        if (fork_result == 0){
            actor_daughter(daughter, id, getpid(), dad->my_pid, children_number, daughter_bank_account[i]);
            return;
        }
    }
    write(events_f, buffer, strlen(buffer));
}

Message make_a_message(MessageType messageType, const char *message) {
    Message msg;
    
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = strlen(message);
    // msg.s_header.s_local_time = time(NULL);
    msg.s_header.s_type = messageType;
<<<<<<< HEAD
    msg.s_header.s_local_time = get_physical_time();
=======
    // msg.s_header.s_local_time = get_physical_time();
>>>>>>> db2b80ce86e764242e52164a15e99bba57f5d8ea
    memset(msg.s_payload, 0, sizeof(msg.s_payload));
    strncpy(msg.s_payload, message, sizeof(msg.s_payload));
    
    return msg;
}

Message make_a_message_2(MessageType messageType, void* message, size_t payload_size) {
    Message msg;
    
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = payload_size;
    // msg.s_header.s_local_time = time(NULL);
    msg.s_header.s_type = messageType;
    msg.s_header.s_local_time = get_physical_time();
    memset(msg.s_payload, 0, sizeof(msg.s_payload));
    memcpy(msg.s_payload, message, sizeof(msg.s_payload));
    
    return msg;
}



int prepare_for_work(struct Actor *daughter){
    Message msg = make_a_message(STARTED, buffer);
    if (send_multicast(daughter, &msg) != 0){
        printf("Write error\n");
        return 1;
    }
    
    int counter = 0;
    while (counter < 10000) {
        int32_t recievers;
        recievers = daughter->my_sisters+1;
        for (int32_t i = 1; i <= recievers; i++) {
            if (daughter->my_id != i && last_recieved_message[i] != STARTED) {
                if (receive(daughter, i, &msg) != 0)
                    break;
                if (msg.s_header.s_type == STARTED)
                    last_recieved_message[i] = STARTED;
            }
        }
        counter++;
        int recivied_started = 0;
        for (int i = 1; i <= (daughter->my_sisters)+1; i++)
            if (last_recieved_message[i] == STARTED && i != daughter->my_id) recivied_started++;

        if (recivied_started == daughter->my_sisters) {
            sprintf(buffer, log_received_all_started_fmt, get_physical_time(), daughter->my_id);
            printf(log_received_all_started_fmt, get_physical_time(), daughter->my_id);
            write(events_f, buffer, strlen(buffer)); 
            return 0;
        }
    }
    if (counter == 10000) {
        // printf("Out of try\n");
        return -1;
    }
    return -1;
}


int at_work(struct Actor *daughter){
    //Some kind of work
    int src, dst;
    balance_t balance_amount;
    Message some_message = make_a_message(ACK, "");
    BalanceState currentBallanceState;
    while (some_message.s_header.s_type != STOP) {
        some_message = make_a_message(ACK, "");
        receive_any(daughter, &some_message);
        if (some_message.s_header.s_type == TRANSFER) {
<<<<<<< HEAD
            TransferOrder order;
            memcpy(&order, some_message.s_payload, sizeof(some_message.s_payload));
            currentBallanceState.s_balance = -1;
            currentBallanceState.s_time = -1;
            
            src = order.s_src;
            dst = order.s_dst;
            balance_amount = order.s_amount;
            
=======
            currentBallanceState.s_balance = -1;
            currentBallanceState.s_time = -1;
            char *ptr = strtok(some_message.s_payload, " ");
            src = atoi(ptr);
            ptr = strtok(NULL, " ");
            dst = atoi(ptr);
            ptr = strtok(NULL, " ");
            balance_amount = (balance_t) atoi(ptr);
            // printf("%d %d %d\n", src, dst, balance_amount);
            char order_string[100];
            sprintf(order_string, "%d %d %d", src, dst, balance_amount);
>>>>>>> db2b80ce86e764242e52164a15e99bba57f5d8ea
            if (daughter->my_id == src) {
                if (balance_amount >= daughter->my_balance.s_balance) {
                    printf("Account %d don't have enough money\n", daughter->my_id);
                    continue;
                }
                daughter->my_balance.s_balance = daughter->my_balance.s_balance - balance_amount;
<<<<<<< HEAD
                some_message = make_a_message_2(TRANSFER, &order, sizeof(TransferOrder));
                
=======
                some_message = make_a_message(TRANSFER, order_string);
>>>>>>> db2b80ce86e764242e52164a15e99bba57f5d8ea
                send(daughter, dst, &some_message);
                currentBallanceState.s_balance = daughter->my_balance.s_balance;
                currentBallanceState.s_time = get_physical_time();
                daughter->history.s_history[currentBallanceState.s_time] = currentBallanceState;
                sprintf(buffer, log_transfer_out_fmt, currentBallanceState.s_time, src, balance_amount, dst);
                printf(buffer, log_transfer_out_fmt, currentBallanceState.s_time, src, balance_amount, dst);
                write(events_f, buffer, strlen(buffer));
            } 
            else if (daughter->my_id == dst) {
                daughter->my_balance.s_balance = daughter->my_balance.s_balance + balance_amount;
                Message tranferDoneMessage = make_a_message(ACK, "");
                send(daughter, PARENT_ID, &tranferDoneMessage);
                currentBallanceState.s_balance = daughter->my_balance.s_balance;
                currentBallanceState.s_time = get_physical_time();
                daughter->history.s_history[currentBallanceState.s_time] = currentBallanceState;
                sprintf(buffer, log_transfer_in_fmt, get_physical_time(), dst, balance_amount, src);
                printf(buffer, log_transfer_in_fmt, get_physical_time(), dst, balance_amount, src);
                write(events_f, buffer, strlen(buffer));
            }
        }
    }
    if (currentBallanceState.s_time == -1)
        daughter->history.s_history_len = MAX_T;
    else
<<<<<<< HEAD
        daughter->history.s_history_len = currentBallanceState.s_time+1;
    
=======
        daughter->history.s_history_len = currentBallanceState.s_time;
>>>>>>> db2b80ce86e764242e52164a15e99bba57f5d8ea
    sprintf(buffer, log_done_fmt, get_physical_time(), daughter->my_id, daughter->my_balance.s_balance);
    return 0;
}


int before_a_sleep(struct Actor *daughter){
    Message done_message = make_a_message(DONE, buffer);
    if (send_multicast(daughter, &done_message) != 0) {
        printf("Write error\n");
        return 1;
    }
    int counter = 0;
    while (counter < 10000) {
        int32_t recievers= daughter->my_sisters+1;
        for (int32_t i = 1; i <= recievers; i++) {
            if (daughter->my_id != i && last_recieved_message[i] != DONE) {
                if (receive(daughter, i, &done_message) != 0)
                    break;
                if (done_message.s_header.s_type == DONE)
                    last_recieved_message[i] = DONE;
            }
        }
        counter++;
        int recivied_done = 0;
        for (int i = 1; i <= (daughter->my_sisters)+1; i++)
            if (last_recieved_message[i] == DONE && i != daughter->my_id) recivied_done++;
            
        if (recivied_done == daughter -> my_sisters) {
            sprintf(buffer, log_received_all_done_fmt, get_physical_time(), daughter->my_id);
            printf(log_received_all_done_fmt, get_physical_time(), daughter->my_id);
            write(events_f, buffer, strlen(buffer));
            return 0;
        }
    }
    if (counter == 10000) {
        // printf("Out of try\n");
        return -1;
    }
    return -1;
}

<<<<<<< HEAD
int check_recieve_again(struct Actor *daughter) {
    int src, dst;
    balance_t balance_amount;
    Message some_message = make_a_message(ACK, "");
    BalanceState currentBallanceState;
    int counter = 0;
    while (counter < 300) {
        some_message = make_a_message(ACK, "");
        receive_any(daughter, &some_message);
        if (some_message.s_header.s_type == TRANSFER) {
            TransferOrder order;
            memcpy(&order, some_message.s_payload, sizeof(some_message.s_payload));
            currentBallanceState.s_balance = -1;
            currentBallanceState.s_time = -1;
            src = order.s_src;
            dst = order.s_dst;
            balance_amount = order.s_amount;

            if (daughter->my_id == src) {
                if (balance_amount >= daughter->my_balance.s_balance) {
                    printf("Account %d don't have enough money\n", daughter->my_id);
                    continue;
                }
                daughter->my_balance.s_balance = daughter->my_balance.s_balance - balance_amount;
                some_message = make_a_message_2(TRANSFER, &order, sizeof(TransferOrder));
                send(daughter, dst, &some_message);
                currentBallanceState.s_balance = daughter->my_balance.s_balance;
                currentBallanceState.s_time = get_physical_time();
                daughter->history.s_history[currentBallanceState.s_time] = currentBallanceState;
                sprintf(buffer, log_transfer_out_fmt, currentBallanceState.s_time, src, balance_amount, dst);
                printf(buffer, log_transfer_out_fmt, currentBallanceState.s_time, src, balance_amount, dst);
                write(events_f, buffer, strlen(buffer));
            } 
            else if (daughter->my_id == dst) {
                daughter->my_balance.s_balance = daughter->my_balance.s_balance + balance_amount;
                Message tranferDoneMessage = make_a_message(ACK, "");
                send(daughter, PARENT_ID, &tranferDoneMessage);
                currentBallanceState.s_balance = daughter->my_balance.s_balance;
                currentBallanceState.s_time = get_physical_time();
                daughter->history.s_history[currentBallanceState.s_time] = currentBallanceState;
                sprintf(buffer, log_transfer_in_fmt, get_physical_time(), dst, balance_amount, src);
                printf(buffer, log_transfer_in_fmt, get_physical_time(), dst, balance_amount, src);
                write(events_f, buffer, strlen(buffer));
            }
        }
        counter++;
    }
    if (currentBallanceState.s_time == -1)
        daughter->history.s_history_len = MAX_T;
    else
        daughter->history.s_history_len = currentBallanceState.s_time+1;
    return 0;
}

int send_balance_history(struct Actor *daughter) {
    for (timestamp_t i = 1; i < daughter->history.s_history_len; i++) {
=======
int send_balance_history(struct Actor *daughter) {
    char history_string[100];
    sprintf(history_string, "%d %d", daughter->history.s_id,daughter->history.s_history_len);
    for (timestamp_t i = 1; i <= daughter->history.s_history_len; i++) {
>>>>>>> db2b80ce86e764242e52164a15e99bba57f5d8ea
        if (daughter->history.s_history[i].s_time == -1) {
            daughter->history.s_history[i].s_time = i;
            daughter->history.s_history[i].s_balance = daughter->history.s_history[i-1].s_balance;
        }
<<<<<<< HEAD
        daughter->history.s_history[i].s_balance_pending_in = 0;
    }
    
    Message balanceHistoryMessage = make_a_message_2(BALANCE_HISTORY, &(daughter->history), sizeof(BalanceHistory));
    send(daughter, PARENT_ID, &balanceHistoryMessage);
=======
        sprintf(history_string, "%s %d %d", history_string ,daughter->history.s_history[i].s_time, daughter->history.s_history[i].s_balance);
    }
    Message balanceHistoryMessage = make_a_message(BALANCE_HISTORY, history_string);
    send(daughter, PARENT_ID, &balanceHistoryMessage);
    printf("%s\n", history_string);
>>>>>>> db2b80ce86e764242e52164a15e99bba57f5d8ea
    return 0;
}

int father_check_started(struct Actor *dad) {
    Message msg = make_a_message(STARTED, buffer);
    int counter = 0;
    while (counter < 10000) {
        int32_t recievers = dad->my_kids;
        for (int32_t i = 1; i <= recievers; i++) {
            if (dad->my_id != i && last_recieved_message[i] != STARTED) {
                if (receive(dad, i, &msg) != 0)
                    break;
                if (msg.s_header.s_type == STARTED)
                    last_recieved_message[i] = STARTED;
            }
        }
        counter++;
        int recivied_started = 0;
        for (int i = 1; i <= (dad->my_kids); i++)
            if (last_recieved_message[i] == STARTED && i != PARENT_ID) recivied_started++;
            
<<<<<<< HEAD
        if (recivied_started == dad->my_kids)
=======
        if (recivied_started == dad->my_kids) {
            // printf(log_received_all_started_fmt, get_physical_time(), dad->my_id);
>>>>>>> db2b80ce86e764242e52164a15e99bba57f5d8ea
            return 0;
    }
    if (counter == 10000) {
        // printf("Out of try\n");
        return -1;
    }
    
    return -1;
}

int father_want_some_sleep(struct Actor *dad) {
    Message done_message = make_a_message(DONE, buffer);
    int counter = 0;
    while (counter < 10000) {
        int32_t recievers = dad->my_kids;
        for (int32_t i = 1; i <= recievers; i++) {
            if (dad->my_id != i && last_recieved_message[i] != DONE) {
                if (receive(dad, i, &done_message) != 0)
                    break;
                if (done_message.s_header.s_type == DONE)
                    last_recieved_message[i] = DONE;
            }
        }
        counter++;
        int recivied_done = 0;
        for (int i = 1; i <= (dad->my_kids); i++)
            if (last_recieved_message[i] == DONE && i != PARENT_ID) recivied_done++;
        if (recivied_done == dad->my_kids){
            // printf(log_received_all_done_fmt, get_physical_time(), dad->my_id);
            return 0;
        }
            
        }
    if (counter == 10000) {
        // printf("Out of try\n");
        return -1;
    }
    return -1;
}

<<<<<<< HEAD
AllHistory historyFormatting(AllHistory history, uint8_t max_history_len) {
    for (uint8_t i = 0; i < history.s_history_len; i++) {
        if (history.s_history[i].s_history_len < max_history_len) {
            for (uint8_t j = history.s_history[i].s_history_len; j < max_history_len+1; j++) {
                history.s_history[i].s_history[j].s_time = j;
                history.s_history[i].s_history[j].s_balance = history.s_history[i].s_history[j-1].s_balance; 
            }
            history.s_history[i].s_history_len = max_history_len;
        }
    }
    print_history(&history);
    return history;
}

int father_get_balance_history(struct Actor *dad) {
    Message done_message = make_a_message(DONE, buffer);
    int counter = 0;
    uint8_t max_history_len = 0;
=======
int father_get_balance_history(struct Actor *dad) {
    Message done_message = make_a_message(DONE, buffer);
    int id, len, counter = 0;
>>>>>>> db2b80ce86e764242e52164a15e99bba57f5d8ea
    AllHistory history;
    history.s_history_len = dad->my_kids;
    while (counter < 10000) {
        int32_t recievers = dad->my_kids;
        for (int32_t i = 1; i <= recievers; i++) {
            if (dad->my_id != i && last_recieved_message[i] != BALANCE_HISTORY) {
                if (receive(dad, i, &done_message) != 0)
                    break;
                if (done_message.s_header.s_type == BALANCE_HISTORY) {
                    last_recieved_message[i] = BALANCE_HISTORY;
                    BalanceHistory currentBallanceHistory;
<<<<<<< HEAD
                    memcpy(&currentBallanceHistory, done_message.s_payload, sizeof(BalanceHistory));
                    history.s_history[currentBallanceHistory.s_id-1] = currentBallanceHistory;
                    if (currentBallanceHistory.s_history_len > max_history_len) {
                        max_history_len = currentBallanceHistory.s_history_len;
                    }
=======
                    char *ptr = strtok(done_message.s_payload, " ");
                    id = atoi(ptr);
                    ptr = strtok(NULL, " ");
                    len = atoi(ptr);
                    currentBallanceHistory.s_id = id;
                    currentBallanceHistory.s_history_len = len;
                    for (int j = 0; j < len; j++) {
                        BalanceState currentState;
                        ptr = strtok(NULL, " ");
                        timestamp_t time = (timestamp_t) atoi(ptr);
                        ptr = strtok(NULL, " ");
                        balance_t balance = (balance_t) atoi(ptr);
                        currentState.s_time = time;
                        currentState.s_balance = balance;
                        currentState.s_balance_pending_in = 0;
                        currentBallanceHistory.s_history[j] = currentState;
                    }
                    history.s_history[id] = currentBallanceHistory;
>>>>>>> db2b80ce86e764242e52164a15e99bba57f5d8ea
                }
            }
        }
        counter++;
        int recivied_history = 0;
        for (int i = 1; i <= (dad->my_kids); i++) 
            if (last_recieved_message[i] == BALANCE_HISTORY && i != PARENT_ID) recivied_history++;
    
        if (recivied_history == dad->my_kids){
<<<<<<< HEAD
            history = historyFormatting(history, max_history_len);
=======
            // printf(log_received_all_done_fmt, get_physical_time(), dad->my_id);
            print_history(&history);
>>>>>>> db2b80ce86e764242e52164a15e99bba57f5d8ea
            return 0;
        }
            
    }
    if (counter == 10000) {
        // printf("Out of try\n");
        return -1;
    }
    return -1;
}
