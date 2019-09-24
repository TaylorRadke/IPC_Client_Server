#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <semaphore.h>
#include <math.h>

#include "shmdef.h"

struct Memory *ShmPtr;

struct thread_args {
    uint32_t number;
    uint32_t *slot;
    int slot_index;
};

int ShmID;

void INTHandler(int);
void cleanup(void);
void listen(struct Memory *);
void *factorise(void *);

void *factorise(void *args){

    struct thread_args *t_args = args;
    uint32_t number = t_args->number;
    uint32_t *slot = t_args->slot;
    int slot_index = t_args->slot_index;

    printf("Factorising %d: ", number);
    int factor = 2;
    int factors = 0;

    while (number > 1 && factor <= number){
        if (number % factor == 0){
            number /= factor;
            factors += 1;
        } else {
            factor += 1;
        }
    }

    pthread_mutex_lock(&(ShmPtr->mutex_slots[slot_index]));
    ShmPtr->slots[slot_index] += factors;
    pthread_mutex_unlock(&(ShmPtr->mutex_slots[slot_index]));

    return NULL;
}

void INTHandler(int sig){
    printf("\rReceived Interrupt signal, deleting shared memory\n");
    cleanup();
    exit(0);
}

void cleanup() {
    printf("Removing Mutexes ... ");
    for (int i = 0; i < 10; i++)
        pthread_mutex_destroy(&(ShmPtr->mutex_slots[i]));
    printf("complete\n");
    
    printf("Detaching shared memory ...");
    shmdt((void *)ShmPtr);
    printf(" complete\n");
    printf("Deleting shared memory  ...");
    shmctl(ShmID, IPC_RMID, NULL);
    printf(" complete\n");
}

int main(int argc, char **argv){
    key_t ShmKey;
    
    ShmKey = ftok("../../", 'x');
    ShmID = shmget(ShmKey, sizeof(struct Memory), IPC_CREAT | 0666);

    if (ShmID < 0){
        printf("*** shmget error (server) : %s ***\n", strerror(errno));
        exit(1);
    }

    ShmPtr = (struct Memory *) shmat(ShmID, NULL, 0);
    if ((int) ShmPtr == -1){
        printf("*** shmat error (server) ***\n");
        exit(1);
    }
    
    signal(SIGINT, INTHandler);

    pthread_t threads[10][32];

    for (int i = 0; i < 10; i++){
        pthread_mutex_init(&(ShmPtr->mutex_slots[i], NULL));
    }

    while (1){
        while (ShmPtr->clientflag == 0);
        
        uint32_t number = ShmPtr->number;
        ShmPtr->clientflag = 0;
        printf("Server: Got %d from client\n", number);
        int slot;


        for (int i = 0; i < 10; i++){
            if (ShmPtr->slots[i] == 0){
                slot = i;
                break;
            }
        }
        
        for (int i = 0; i < 32 && pow(2,i) <= number; i++){
            struct thread_args *args = malloc(sizeof(struct thread_args));

            args->number = number >> i;
            args->slot = &(ShmPtr->slots[i]);
            args->slot_index = slot;

            if (pthread_create(&threads[slot][i], NULL, factorise, args)){
                printf("*** thread create error (server) : %s ***\n", strerror(errno));
                exit(1);
            }

            if (pthread_join( threads[slot][i], NULL)){
                printf("*** thread join error (server) : %s ***\n", strerror(errno));
                exit(1);
            }
        }
        

        printf("factors found for input %d: %d\n", number, ShmPtr->slots[slot]);
    }

    cleanup();
}