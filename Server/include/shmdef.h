#ifndef _SHMDEF_H
#define _SHMDEF_H

#include <stdint.h>
#include <pthread.h>

#define EMPTY 0
#define FILLED 1

struct Memory {
    //Server partition of shared memory
    pthread_mutex_t server_mutex[10];
    uint8_t server_flag[10];
    uint8_t assigned[10];
    uint32_t numbers[10];
    uint32_t slots[10];
    uint64_t timestamps[10];

    //Client partition of shared memory 
    pthread_mutex_t client_mutex;
    uint8_t client_flag;
    uint32_t number;
    uint64_t timestamp;
};

#endif