#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/time.h>

#include "shmdef.h"

struct Memory *ShmPtr;


void error(char *err, char *source){
    printf("*** %s ( Client - %s) %s ***\n", err, source, strerror(errno));
    exit(1);
}

uint64_t get_timestamp(){
    time_t now;
    time(&now);
    return now;
}

void lock_client(){
    pthread_mutex_lock(&(ShmPtr->client_mutex));
}

void unlock_client(){
    pthread_mutex_unlock(&((ShmPtr->client_mutex)));
}

void lock_server_slot(int slot){
    pthread_mutex_lock(&(ShmPtr->server_mutex[slot]));
}

void unlock_server_slot(int slot){
    pthread_mutex_unlock(&(ShmPtr->server_mutex[slot]));
}

uint8_t read_client_flag(){
    lock_client();
    uint8_t flag = ShmPtr->client_flag;
    unlock_client();
    return flag;
}

void write_client_number(uint32_t number){
    while (read_client_flag() != 0) usleep(10);

    lock_client();
    ShmPtr->number = number;
    ShmPtr->client_flag = 1;
    ShmPtr->timestamp = get_timestamp();
    unlock_client();
    printf("Sent input\n");

}

uint8_t read_server_flag(int slot){
    lock_server_slot(slot);
    uint8_t flag = ShmPtr->server_flag[slot];
    ShmPtr->server_flag[slot] = 0;
    unlock_server_slot(slot);
    return flag;
}

void read_server_slot(int slot, uint32_t response[2]){
    lock_server_slot(slot);
    response[0] = ShmPtr->slots[slot];
    response[1] = ShmPtr->numbers[slot];
    unlock_server_slot(slot);
}

int count_slots_assigned(){
    int count = 0;
    for (int i = 0; i < 10; i++){
        lock_server_slot(i);
        count += ShmPtr->assigned[i];
        unlock_server_slot(i);
    }
    return count;
}

void *listenStdin(){
    struct pollfd inputFD;

    inputFD.events = POLLIN;
    inputFD.fd = 0;
    char input[256];
    
    while (1) {
        bzero(input, strlen(input));
        poll(&inputFD, 1, -1);
        scanf("%s", input);

        if (input[0] == 'q' || !(strcmp("quit", input))){
            printf("exiting...\n");
            exit(0);
        }

        if (count_slots_assigned() < 10){
            while (read_client_flag() != 0) usleep(10);
            write_client_number(atoi(input));
        } else {
            printf("Warning: Server is currently busy at this time\n");
        }
    }
}

void *watchServerFlags(void *args){
    while (1) {
        for (int i = 0; i < 10; i++){
            if (read_server_flag(i) == 1){
                uint32_t response[2];
                read_server_slot(i, response);
                printf("\rClient: Server got factor %d for input %d\n", response[0], response[1]);
            }
        }
    }
}

// void *watchServerRequestComplete(void *args){
//     while(1);
//     // while (1){
//         // lock_client();
//         // printf("Client: Server finished request %d for input %d\n", ShmPtr->request_complete, ShmPtr->numbers[ShmPtr->request_complete]);
//         // ShmPtr->request_complete = -1;
//         // pthread_mutex_unlock(&(ShmPtr->client_mutex));
//     // }
    
// }


int main(int argc, char **argv){
    key_t ShmKey;
    int ShmID;
    pthread_t input_thread, serverflag_thread, requestcomplete_thread;

    ShmKey = ftok("../../", 'x');
    
    ShmID = shmget(ShmKey, sizeof(struct Memory), 0666);
    if (ShmID < 0)
        error("shmget error", "shmget");
    
    ShmPtr = (struct Memory *) shmat(ShmID, NULL, 0);
    if ((int) ShmPtr == -1)
        error("shmat error", "shmat");

    if (pthread_create(&input_thread, NULL, listenStdin, NULL))
        error("thread create error", "input thread");
    
    if (pthread_create(&serverflag_thread, NULL, watchServerFlags, NULL))
        error("thread create error", "watchServerFlags thread");


    // if (pthread_create(&requestcomplete_thread, NULL, watchServerRequestComplete, NULL))
    //     error("thread create error", "watchRequestComplete thread");

    if (pthread_join(input_thread, NULL))
        error("thread join error", "input thread");

    if (pthread_join(serverflag_thread, NULL))
        error("thread join error", "server_flag thread");

    // if (pthread_join(requestcomplete_thread, NULL))
    //     error("thread join error", "requestcomplete thread");

    printf("Client has detached from shared memory\n");
    printf("Client exits ...\n");
    exit(0);
}