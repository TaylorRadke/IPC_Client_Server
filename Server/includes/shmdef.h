#include <stdint.h>

#define EMPTY 0
#define FILLED 1
#define QUIT 2

struct Memory {
    uint8_t clientflag;
    uint8_t serverflag[10];
    uint32_t number;
    uint32_t numbers;
    uint64_t timestamp;
    uint32_t slots[10];
    uint64_t timestamps[10];
};