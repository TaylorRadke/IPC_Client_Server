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
    int slot;
};

int ShmID;
int assigned_slots[10] = {0};
pthread_mutex_t assigned_slots_mutex[10];

void INTHandler(int);
void cleanup(void);
void listen(struct Memory *);
void *factorise(void *);

void error(char *msg){
    printf("*** %s (server) %s ***\n", msg, strerror(errno));
    exit(1);
}

void *factorise(void *args){

    struct thread_args *t_args = args;
    uint32_t number = t_args->number;
    int slot_index = t_args->slot;

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
    printf("\rReceived Interrupt signal, cleaning up before terminating\n");
    cleanup();
    exit(0);
}

void *factorsParentThread(void *args){
    struct thread_args *t_args = args;
    uint32_t number = t_args->number;
    int slot = t_args->slot;
    pthread_t threads[32];

    for (int i = 0; i < 32 && pow(2,i) <= number; i++){
        struct thread_args *f_args = malloc(sizeof(struct thread_args));

        f_args->number = number >> i;
        f_args->slot = slot;

        if (pthread_create(&threads[i], NULL, factorise, f_args)){
            error("thread create error");
        }

        if (pthread_join(threads[i], NULL)){
            error("thread join error");
        }
    }

    printf("Server, thread %d: Factors found for input %d: %d\n", slot, number, ShmPtr->slots[slot]);
    
    sleep(5);
    ShmPtr->serverflag[slot] = 1;

    while (ShmPtr->serverflag[slot] == 1);

    pthread_mutex_lock(&assigned_slots_mutex[slot]);
    assigned_slots[slot] = 0;
    // ShmPtr->numbers[slot] = 0;
    ShmPtr->slots[slot] = 0;
    pthread_mutex_unlock(&assigned_slots_mutex[slot]);
    
    pthread_mutex_lock(&(ShmPtr->processing_mutex));
    ShmPtr->processing -= 1;
    pthread_mutex_unlock(&(ShmPtr->processing_mutex));
    return NULL;
}

void cleanup() {
    printf("Destroying Mutexes... ");
    for (int i = 0; i < 10; i++)
        pthread_mutex_destroy(&(ShmPtr->mutex_slots[i]));
    printf("complete\n");
    
    printf("Detaching shared memory...");
    shmdt((void *)ShmPtr);
    printf(" complete\n");
    printf("Deleting shared memory...");
    shmctl(ShmID, IPC_RMID, NULL);
    printf(" complete\n");
}

int main(int argc, char **argv){
    key_t ShmKey;
    pthread_t factor_threads[10][32];
    pthread_t threads[10];

    signal(SIGINT, INTHandler);
    

    ShmKey = ftok("../../", 'x');
    ShmID = shmget(ShmKey, sizeof(struct Memory), IPC_CREAT | 0666);

    if (ShmID < 0){
        error("shmget error");
    }

    ShmPtr = (struct Memory *) shmat(ShmID, NULL, 0);
    if ((int) ShmPtr == -1){
        error("shmat error");
    }

    pthread_mutex_init(&(ShmPtr->processing_mutex), NULL);
    ShmPtr->processing = 0;

    for (int i = 0; i < 10; i++){
        pthread_mutex_init(&(ShmPtr->mutex_slots[i]), NULL);
        pthread_mutex_init(&assigned_slots_mutex[i], NULL);
    }

    while (1){
        while (ShmPtr->clientflag == 0);
        
        pthread_mutex_lock(&(ShmPtr->processing_mutex));
        ShmPtr->processing += 1;
        pthread_mutex_unlock(&(ShmPtr->processing_mutex));

        pthread_mutex_lock(&(ShmPtr->client_mutex));
        uint32_t number = ShmPtr->number;
        ShmPtr->clientflag = 0;
        pthread_mutex_unlock(&(ShmPtr->client_mutex));
    
        int slot;
        for (int i = 0; i < 10; i++){
            pthread_mutex_lock(&assigned_slots_mutex[i]);
            int slot_value = assigned_slots[i];
            pthread_mutex_unlock(&assigned_slots_mutex[i]);

            if (slot_value == 0){
                slot = i;
                break;
            }
        }

        pthread_mutex_lock(&assigned_slots_mutex[slot]);
        assigned_slots[slot] = 1;
        ShmPtr->numbers[slot] = number;
        pthread_mutex_unlock(&assigned_slots_mutex[slot]);
    
        struct thread_args *args = malloc(sizeof(struct thread_args));
        args->number = number;
        args->slot = slot;

        if (pthread_create(&threads[slot], NULL, factorsParentThread, args)){
            error("thread create error");
        }
    }
    cleanup();
}