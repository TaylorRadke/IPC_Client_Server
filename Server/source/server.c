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
int ShmID;

struct thread_args {
    uint32_t number;
    int slot;
    uint32_t factor;
};

struct read_args {
    pthread_mutex_t read_mutex;
};

void INTHandler(int);
void cleanup(void);
void listen(struct Memory *);
void *factorise(void *);

void error(char *err, char *source){
    printf("*** %s ( Server - %s) %s ***\n", err, source, strerror(errno));
    exit(1);
}

void write_factor(int slot, uint32_t factor){
    while (ShmPtr->serverflag[slot] != 0);
    pthread_mutex_lock(&(ShmPtr->server_mutex[slot]));
    ShmPtr->slots[slot] = factor;
    ShmPtr->serverflag[slot] = 1;
    pthread_mutex_unlock(&(ShmPtr->server_mutex[slot]));
}

void *factorise(void *args){
    struct thread_args *t_args = args;
    uint32_t number = t_args->number;
    int slot_index = t_args->slot;
    uint32_t factor = t_args->factor;
    
    uint32_t o_number = number;

    while (number > factor && number > 1 && factor > 1){
        if (number % factor == 0){
            number /= factor;
            write_factor(slot_index, factor);
            printf("Thread %d, number %d: Found factor %d\n", slot_index, o_number, factor);
        } else {
            factor += 1;
        }
    }
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

        f_args->number = number;
        f_args->slot = slot;
        f_args->factor = number >> i;

        if (pthread_create(&threads[i], NULL, factorise, f_args)){
            error("thread create error", "factorise thread");
        }

        if (pthread_join(threads[i], NULL)){
            error("thread join error", "thread");
        }
    }

    while (ShmPtr->serverflag[slot] != 0);
    return NULL;
}

void cleanup() {
    printf("Destroying Mutexes... ");
    for (int i = 0; i < 10; i++)
        pthread_mutex_destroy(&(ShmPtr->server_mutex[i]));
    printf("complete\n");
    
    printf("Detaching shared memory...");
    shmdt((void *)ShmPtr);
    printf(" complete\n");
    printf("Deleting shared memory...");
    shmctl(ShmID, IPC_RMID, NULL);
    printf(" complete\n");
}
void initiliseShm(void){
    key_t ShmKey;

    ShmKey = ftok("../../", 'x');
    ShmID = shmget(ShmKey, sizeof(struct Memory), IPC_CREAT | 0666);

    if (ShmID < 0){
        error("shmget error", "shmget");
    }

    ShmPtr = (struct Memory *) shmat(ShmID, NULL, 0);
    if ((int) ShmPtr == -1){
        error("shmat error", "shmat");
    }

    ShmPtr->request_complete = -1;
}
int main(int argc, char **argv){
    pthread_t factor_threads[10][32];
    pthread_t threads[10];

    initiliseShm();

    signal(SIGINT, INTHandler);

    for (int i = 0; i < 10; i++){
        pthread_mutex_init(&(ShmPtr->server_mutex[i]), NULL);
    }

    while (1){
        while (ShmPtr->clientflag == 0);
        printf("%d\n", ShmPtr->clientflag);
        if (ShmPtr->request_complete == 1) continue;

        uint32_t number, slot;

        pthread_mutex_lock(&(ShmPtr->client_mutex));
        number = ShmPtr->number;
        ShmPtr->clientflag = 0;
        pthread_mutex_unlock(&(ShmPtr->client_mutex));
    
        for (int i = 0; i < 10; i++){
            pthread_mutex_lock(&(ShmPtr->server_mutex[i]));
            int slot_value = ShmPtr->assigned[i];

            if (slot_value == 0){
                slot = i;
                ShmPtr->assigned[slot] = 1;
                ShmPtr->numbers[slot] = number;
                break;
            }
            pthread_mutex_unlock(&(ShmPtr->server_mutex[i]));
        }
        pthread_mutex_unlock(&(ShmPtr->server_mutex[slot]));
    
        struct thread_args *args = malloc(sizeof(struct thread_args));
        args->number = number;
        args->slot = slot;

        if (pthread_create(&threads[slot], NULL, factorsParentThread, args))
            error("thread create error", "factorsParentThread");

        if (pthread_join(threads[slot], NULL))
            error("thread join error", "factorsParentThread");

        while (ShmPtr->request_complete != -1);

        pthread_mutex_lock(&(ShmPtr->client_mutex));
        ShmPtr->request_complete = slot;
        pthread_mutex_unlock(&(ShmPtr->client_mutex));

        while (ShmPtr->request_complete != -1);

        pthread_mutex_lock(&(ShmPtr->server_mutex[slot]));
        ShmPtr->assigned[slot] = 0;
        pthread_mutex_unlock(&(ShmPtr->server_mutex[slot]));
    }
    cleanup();
}