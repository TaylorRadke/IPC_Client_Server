#ifndef _SHMDEF_H
#define _SHMDEF_H

#include <stdint.h>
#include <pthread.h>

#define MAX_QUERIES 10
#define MAX_BITSIZE (sizeof(uint32_t) * 8)

struct Memory {
    //Server partition of shared memory
    pthread_mutex_t server_mutex[MAX_QUERIES];
    uint8_t server_flag[MAX_QUERIES];
    uint8_t assigned[MAX_QUERIES];
    uint32_t numbers[MAX_QUERIES];
    uint32_t slots[MAX_QUERIES];
    uint64_t timestamps[MAX_QUERIES];

    //Client partition of shared memory 
    pthread_mutex_t client_mutex;
    uint8_t client_flag;
    uint32_t number;
    uint64_t timestamp;
};

#endif