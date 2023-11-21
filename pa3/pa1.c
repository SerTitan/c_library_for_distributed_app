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
static timestamp_t current_logical_time = 0;

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
    dad->my_balance.s_balance = 0;
}

void actor_daughter(struct Actor *daughter, local_id id, pid_t pid, pid_t father_pid, int32_t children_number, balance_t balance) {
    daughter->my_id = id;
    daughter->my_pid = pid;
    daughter->my_father_pid = father_pid;
    daughter->my_kids = 0;
    daughter->my_sisters = children_number-1;
    daughter->my_balance.s_balance = balance;
    daughter->my_balance.s_time = 0;
    daughter->my_balance.s_balance_pending_in = 0;
    daughter->history.s_id = id;
    daughter->history.s_history[0] = daughter->my_balance;
    BalanceState nullBalance;
    nullBalance.s_balance = -1;
    nullBalance.s_time = -1;
    nullBalance.s_balance_pending_in = 0;
    for (int i = 1; i < MAX_T; i++) {
        daughter->history.s_history[i] = nullBalance;
    }
    sprintf(buffer, log_started_fmt, current_logical_time, daughter->my_id, daughter->my_pid, daughter->my_father_pid, daughter->my_balance.s_balance);
    printf(log_started_fmt, current_logical_time, daughter->my_id, daughter->my_pid, daughter->my_father_pid, daughter->my_balance.s_balance);
}

// pid named dad make fork kids times 
void become_a_dad(struct Actor *dad, struct Actor *daughter, int32_t children_number, balance_t* daughter_bank_account){
    //first_daughter
    local_id id = PARENT_ID + 1;
    uint8_t fork_result = fork();
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
        if (fork_result == 0 && getpid() != dad->my_pid){
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

TransferOrder makeTransferOrder(local_id s_src, local_id s_dst, balance_t s_amount) {
    TransferOrder order;
    order.s_amount = s_amount;
    order.s_dst = s_dst;
    order.s_src = s_src;

    return order;
}

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount) {
    struct Actor *father = (struct Actor *)parent_data;
    TransferOrder order = makeTransferOrder(src, dst, amount);
    char order_string[100];

    sprintf(order_string, "%d %d %d", order.s_src, order.s_dst, order.s_amount);
    current_logical_time++;

    Message to_src_message = make_a_message_2(TRANSFER, &order, sizeof(TransferOrder));
    send(father, src, &to_src_message);

    while (to_src_message.s_header.s_type != ACK)
        receive(father, dst, &to_src_message);
    current_logical_time = max(current_logical_time, to_src_message.s_header.s_local_time) + 1;
}

int started_to_send(struct Actor *daughter) {
    current_logical_time++;

    Message msg = make_a_message(STARTED, buffer);

    if (send_multicast(daughter, &msg) != 0){
        printf("Write error\n");
        return 1;
    }
    
    return 0;
}

int prepare_for_work(struct Actor *daughter){
    Message msg = make_a_message(ACK, buffer);
    int counter = 0;
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
        counter++;
        uint8_t recivied_started = 0;
        for (uint8_t i = 1; i <= (daughter->my_sisters)+1; i++)
            if (last_recieved_message[i] == STARTED && i != daughter->my_id) recivied_started++;

        if (recivied_started == daughter->my_sisters) {
            sprintf(buffer, log_received_all_started_fmt, current_logical_time, daughter->my_id);
            printf(log_received_all_started_fmt, current_logical_time, daughter->my_id);
            write(events_f, buffer, strlen(buffer)); 
            return 0;
        }
        if (counter == 10000) return 1;
    }

    return 1;
}


int at_work(struct Actor *daughter){
    //Some kind of work
    int src, dst;
    balance_t balance_amount;
    Message some_message;
    BalanceState currentBallanceState;
    while (some_message.s_header.s_type != STOP) {
        some_message = make_a_message(ACK, "");
        receive_any(daughter, &some_message);
        if (some_message.s_header.s_type == TRANSFER) {
            TransferOrder order;
            memcpy(&order, some_message.s_payload, sizeof(some_message.s_payload));
            currentBallanceState.s_balance = -1;
            currentBallanceState.s_time = -1;
            current_logical_time = max(current_logical_time, some_message.s_header.s_local_time) + 1;
            src = order.s_src;
            dst = order.s_dst;
            balance_amount = order.s_amount;
            
            if (daughter->my_id == src) {
                if (balance_amount >= daughter->my_balance.s_balance) {
                    printf("Account %d don't have enough money\n", daughter->my_id);
                    continue;
                }
                daughter->my_balance.s_balance = daughter->my_balance.s_balance - balance_amount;
                current_logical_time++;
                some_message = make_a_message_2(TRANSFER, &order, sizeof(TransferOrder));
                
                send(daughter, dst, &some_message);
                currentBallanceState.s_balance = daughter->my_balance.s_balance;
                currentBallanceState.s_time = current_logical_time;
                currentBallanceState.s_balance_pending_in = balance_amount;
                daughter->history.s_history[currentBallanceState.s_time] = currentBallanceState;
                sprintf(buffer, log_transfer_out_fmt, currentBallanceState.s_time, src, balance_amount, dst);
                printf(log_transfer_out_fmt, currentBallanceState.s_time, src, balance_amount, dst);
                write(events_f, buffer, strlen(buffer));
                currentBallanceState.s_time++;
                daughter->history.s_history[currentBallanceState.s_time] = currentBallanceState;
            } 
            else if (daughter->my_id == dst) {
                daughter->my_balance.s_balance = daughter->my_balance.s_balance + balance_amount;
                current_logical_time++;
                Message tranferDoneMessage = make_a_message(ACK, "");
                send(daughter, PARENT_ID, &tranferDoneMessage);
                currentBallanceState.s_balance = daughter->my_balance.s_balance;
                currentBallanceState.s_time = current_logical_time;
                daughter->history.s_history[currentBallanceState.s_time] = currentBallanceState;
                sprintf(buffer, log_transfer_in_fmt, current_logical_time, dst, balance_amount, src);
                printf(log_transfer_in_fmt, current_logical_time, dst, balance_amount, src);
                write(events_f, buffer, strlen(buffer));
            }
        }
    }
    current_logical_time = max(current_logical_time, some_message.s_header.s_local_time) + 1;
    if (currentBallanceState.s_time == -1)
        daughter->history.s_history_len = MAX_T;
    else
        daughter->history.s_history_len = current_logical_time - 1;
    printf(log_done_fmt, current_logical_time, daughter->my_id, daughter->my_balance.s_balance);
    sprintf(buffer, log_done_fmt, current_logical_time, daughter->my_id, daughter->my_balance.s_balance);
    return 0;
}

int done_to_send(struct Actor *daughter) {
    current_logical_time++;

    Message done_message = make_a_message(DONE, "");
    if (send_multicast(daughter, &done_message) != 0) 
        return 1;

    return 0;
}

void print_recieved_message(struct Actor *daughter) {
    printf("Process's %d last receivied messages are: ", daughter->my_id);
    for (int i = 0; i <= daughter->my_sisters+1; i++) {
        printf("%d ", last_recieved_message[i]);
    }
    printf("\n");
}

int before_a_sleep(struct Actor *daughter){
    Message done_message = make_a_message(ACK, buffer);
    int32_t counter = 0;
    // int flag = 0;
    while (1) {
        for (uint8_t i = 1; i <= daughter->my_sisters+1; i++) {
            if (daughter->my_id != i && last_recieved_message[i] != DONE) {
                if (receive(daughter, i, &done_message) != 0) {
                    break;
                }
                if (done_message.s_header.s_type == DONE) {
                    last_recieved_message[i] = DONE;
                    current_logical_time = max(current_logical_time, done_message.s_header.s_local_time) + 1;
                }
            }
        }
        // if (flag % 50 == 0 && flag <= 500) {
        //     print_recieved_message(daughter);
        // }
        // flag++;
        counter++;

        uint8_t recivied_done = 0;
        for (uint8_t i = 1; i <= (daughter->my_sisters)+1; i++)
            if (last_recieved_message[i] == DONE && i != daughter->my_id) recivied_done++;
        
        if (recivied_done == daughter->my_sisters) {
            sprintf(buffer, log_received_all_done_fmt, current_logical_time, daughter->my_id);
            printf(log_received_all_done_fmt, current_logical_time, daughter->my_id);
            write(events_f, buffer, strlen(buffer));
            return 0;
        }
        
        if (counter == 10000) return 1;
    }
    return 1;
}

int check_recieve_again(struct Actor *daughter) {
    int src, dst;
    balance_t balance_amount;
    BalanceState currentBallanceState;
    int32_t counter = 0;
    while (1) {
        Message some_message = make_a_message(ACK, "");
        receive_any(daughter, &some_message);
        if (some_message.s_header.s_type == TRANSFER) {
            TransferOrder order;
            memcpy(&order, some_message.s_payload, sizeof(some_message.s_payload));
            currentBallanceState.s_balance = -1;
            currentBallanceState.s_time = -1;
            current_logical_time = max(current_logical_time, some_message.s_header.s_local_time) + 1;
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
                currentBallanceState.s_time = current_logical_time;
                daughter->history.s_history[currentBallanceState.s_time] = currentBallanceState;
                currentBallanceState.s_balance_pending_in = 0;
                currentBallanceState.s_time = current_logical_time + 1;
                daughter->history.s_history[currentBallanceState.s_time] = currentBallanceState;
                sprintf(buffer, log_transfer_out_fmt, currentBallanceState.s_time, src, balance_amount, dst);
                printf(buffer, log_transfer_out_fmt, currentBallanceState.s_time, src, balance_amount, dst);
                write(events_f, buffer, strlen(buffer));
            } 
            else if (daughter->my_id == dst) {
                daughter->my_balance.s_balance = daughter->my_balance.s_balance + balance_amount;
                current_logical_time++;
                Message tranferDoneMessage = make_a_message(ACK, "");
                send(daughter, PARENT_ID, &tranferDoneMessage);
                currentBallanceState.s_balance = daughter->my_balance.s_balance;
                currentBallanceState.s_time = current_logical_time;
                daughter->history.s_history[currentBallanceState.s_time] = currentBallanceState;
                sprintf(buffer, log_transfer_in_fmt, current_logical_time, dst, balance_amount, src);
                printf(buffer, log_transfer_in_fmt, current_logical_time, dst, balance_amount, src);
                write(events_f, buffer, strlen(buffer));
            }
        }
        counter++;
        if (counter == 10000) return 1;
    }
    
    return 0;
}

int send_balance_history(struct Actor *daughter) {
    for (timestamp_t i = 1; i < daughter->history.s_history_len; i++) {
        if (daughter->history.s_history[i].s_time == -1) {
            daughter->history.s_history[i].s_time = i;
            daughter->history.s_history[i].s_balance = daughter->history.s_history[i-1].s_balance;
        }
        if (daughter->history.s_history[i].s_balance > daughter->history.s_history[i-1].s_balance) {
            daughter->history.s_history[i].s_balance_pending_in = 0;
        }
    }
    current_logical_time++;
    Message balanceHistoryMessage = make_a_message_2(BALANCE_HISTORY, &(daughter->history), sizeof(BalanceHistory));
    send(daughter, PARENT_ID, &balanceHistoryMessage);
    return 0;
}

int father_check_started(struct Actor *dad) {
    current_logical_time++;
    Message msg = make_a_message(STARTED, buffer);
    int32_t counter = 0;
    while (1) {
        for (int32_t i = 1; i <= dad->my_kids; i++) {
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

        counter++;
        if (counter == 10000) return 1;
    }
    
    return 1;
}

int father_want_some_sleep(struct Actor *dad) {
    current_logical_time++;
    Message done_message = make_a_message(DONE, buffer);
    int32_t counter = 0;
    while (1) {
        int32_t recievers = dad->my_kids;
        for (int32_t i = 1; i <= recievers; i++) {
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
        counter++;
        if (counter == 10000) return 1;
    }
    
    return 1;
}

AllHistory historyFormatting(AllHistory history, uint8_t max_history_len) {
    /*
    for (uint8_t i = 0; i < history.s_history_len; i++) {
        if (history.s_history[i].s_history_len < max_history_len) {
            for (uint8_t j = history.s_history[i].s_history_len; j <= max_history_len+1; j++) {
                history.s_history[i].s_history[j].s_time = j;
                history.s_history[i].s_history[j].s_balance = history.s_history[i].s_history[j-1].s_balance; 
            }
            history.s_history[i].s_history_len = max_history_len;
        }
    }
    */
    print_history(&history);
    return history;
}

int father_get_balance_history(struct Actor *dad) {
    Message done_message = make_a_message(DONE, buffer);
    int32_t counter = 0;
    uint8_t max_history_len = 0;
    AllHistory history;
    history.s_history_len = dad->my_kids;
    while (1) {
        for (int32_t i = 1; i <= dad->my_kids; i++) {
            if (dad->my_id != i && last_recieved_message[i] != BALANCE_HISTORY) {
                if (receive(dad, i, &done_message) != 0)
                    break;
                if (done_message.s_header.s_type == BALANCE_HISTORY) {
                    last_recieved_message[i] = BALANCE_HISTORY;
                    current_logical_time = max(current_logical_time, done_message.s_header.s_local_time) + 1;
                    BalanceHistory currentBallanceHistory;
                    memcpy(&currentBallanceHistory, done_message.s_payload, sizeof(BalanceHistory));
                    history.s_history[currentBallanceHistory.s_id-1] = currentBallanceHistory;
                    if (currentBallanceHistory.s_history_len > max_history_len)
                        max_history_len = currentBallanceHistory.s_history_len;
                }
            }
        }
        uint8_t recivied_history = 0;
        for (uint8_t i = 1; i <= (dad->my_kids); i++) 
            if (last_recieved_message[i] == BALANCE_HISTORY && i != PARENT_ID) recivied_history++;
    
        if (recivied_history == dad->my_kids){
            history = historyFormatting(history, max_history_len);
            return 0;
        }
        counter++;
        if (counter == 10000) return 1;
    }
    return 1;
}
