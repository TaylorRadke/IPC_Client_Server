#include <stdint.h>
#include <pthread.h>

#define EMPTY 0
#define FILLED 1
#define QUIT 2

struct Memory {
    uint8_t clientflag;
    int processing;
    pthread_mutex_t processing_mutex;
    uint8_t serverflag[10];
    uint32_t number;
    uint32_t numbers;
    uint64_t timestamp;
    uint64_t timestamps[10];
    uint32_t slots[10];
    pthread_mutex_t mutex_slots[10];
};