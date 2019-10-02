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
    struct timeval now;
    gettimeofday(&now, NULL);

    uint64_t millis = (now.tv_sec * (uint64_t) 1000) + (now.tv_usec / 1000);
    return millis;
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
    for (int i = 0; i < MAX_QUERIES; i++){
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
        
        if (input[0] == '-'){
            printf("Warning: Negative numbers are not accepted. Please provide a non-negative number.\n");
            continue;
        }

        if (strtoul(input, NULL, 10) > UINT32_MAX || strtoul(input, NULL, 10) < 0){
            printf("Warning: Number is too large, please provide a number less than %u\n", UINT32_MAX);
            continue;
        }
    
        uint32_t number = (uint32_t) strtoul(input, NULL, 10);
        
        if (number < 0){
            printf("Negative number\n");
        }

    
        if (number < 0){
            printf("Error: Given input exceeds maximum allowed size for a %lu bit integer\n", MAX_BITSIZE);
            break;
        }
        if (count_slots_assigned() < MAX_QUERIES){
            while (read_client_flag() != 0) usleep(10);
            write_client_number(number);
        } else {
            printf("Warning: Server is currently busy\n");
        }
    }

    return NULL;
}

void *watchServerFlags(void *args){
    while (1) {
        for (int i = 0; i < MAX_QUERIES; i++){
            uint8_t flag = read_server_flag(i);
            if (flag == 1){
                //New factor for request
                uint32_t response[2];
                read_server_slot(i, response);
                printf("\rClient [%d], input %u: %u\n", i, response[1], response[0]);
            } else if (flag == 2){
                //Request is complete
                lock_server_slot(i);
                uint64_t now = get_timestamp();
                uint64_t req_time = ShmPtr->timestamps[i];
                uint32_t number = ShmPtr->numbers[i];
                ShmPtr->server_flag[i] = 0;
                unlock_server_slot(i);

                printf("Client [%d]: Request complete... %.3lfs\n\n", i, (double) (now - req_time)/1000);
            }

            
        }
        usleep(10);
    }
}


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

    if (pthread_join(input_thread, NULL))
        error("thread join error", "input thread");

    if (pthread_join(serverflag_thread, NULL))
        error("thread join error", "server_flag thread");

    exit(0);
}