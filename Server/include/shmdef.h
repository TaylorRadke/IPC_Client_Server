#ifndef _SHMDEF_H
#define _SHMDEF_H

#include <stdint.h>
#include <pthread.h>

#define EMPTY 0
#define FILLED 1
#define QUIT 2

struct Memory {
    uint8_t clientflag;
    uint8_t serverflag[10];
    uint8_t assigned[10];
    uint32_t number;
    int32_t request_complete;
    uint32_t numbers[10];
    uint32_t slots[10];
    uint64_t timestamp;
    uint64_t timestamps[10];
    pthread_mutex_t client_mutex;
    pthread_mutex_t server_mutex[10];
};

#endif