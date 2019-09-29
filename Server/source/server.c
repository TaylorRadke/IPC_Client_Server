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
#include <time.h>

#include "shmdef.h"

struct Memory *ShmPtr;
int ShmID;

struct thread_args {
    uint32_t number;
    int slot;
    uint32_t factor;
};

void INTHandler(int);
void cleanup(void);
void listen(struct Memory *);
void *factorise(void *);
void write_factor(int index, uint32_t factor);
void lock_client();
void unlock_client();
void lock_server_slot(int);
void unlock_server_slot(int);
void error(char *, char *);
uint8_t read_client_flag();


void error(char *err, char *source){
    printf("*** %s ( Server - %s) %s ***\n", err, source, strerror(errno));
    cleanup();
    exit(1);
}

uint8_t read_client_flag(){
    lock_client();
    uint8_t client_flag = ShmPtr->client_flag;
    unlock_client();
    return client_flag;
}

void lock_server_slot(int slot){ 
    pthread_mutex_lock(&(ShmPtr->server_mutex[slot])); 
}

void unlock_server_slot(int slot){ 
    pthread_mutex_unlock(&(ShmPtr->server_mutex[slot]));
}

void lock_client(){
    pthread_mutex_lock(&(ShmPtr->client_mutex));
}

void unlock_client(){
    pthread_mutex_unlock(&(ShmPtr->client_mutex));
}

uint32_t read_client_number(){
    lock_client();
    uint32_t number = ShmPtr->number;
    ShmPtr->client_flag = 0;
    unlock_client();
    return number;
}

uint8_t read_server_slot(int slot){
    lock_server_slot(slot);
    uint8_t flag = ShmPtr->server_flag[slot];
    unlock_server_slot(slot);
    return flag;
}

void notify_request_complete(int slot){
    while (read_server_slot(slot) != 0) usleep(10);
    lock_server_slot(slot);
    ShmPtr->server_flag[slot] = 2;
    unlock_server_slot(slot);
}

void write_factor(int slot, uint32_t factor){
    while (read_server_slot(slot) != 0) usleep(10);
    lock_server_slot(slot);
    ShmPtr->slots[slot] = factor;
    ShmPtr->server_flag[slot] = 1;
    unlock_server_slot(slot);
}

int assign_slot(uint32_t number){
    int slot;
    for (int i = 0; i < MAX_QUERIES; i++){
        lock_server_slot(i);
        int assigned = ShmPtr->assigned[i];
        unlock_server_slot(i);
        if (!assigned){
            slot = i;
            lock_server_slot(i);
            ShmPtr->assigned[slot] = 1;
            ShmPtr->numbers[slot] = number;
            ShmPtr->timestamps[slot] = ShmPtr->timestamp;
            unlock_server_slot(i);
            printf("Assigned slot %d\n", slot);
            return slot;
        } 
    }
    error("too many running requests", "slot assignment");
    return -1;
}

void unassign_slot(int slot){
    while (read_server_slot(slot) != 0) usleep(10);

    lock_server_slot(slot);
    ShmPtr->assigned[slot] = 0;
    unlock_server_slot(slot);
    printf("Unassigned slot %d\n", slot);
}

void *factorise(void *args){
    struct thread_args *t_args = args;
    uint32_t number = t_args->number;
    int slot = t_args->slot;
    uint32_t factor = t_args->factor;
    uint32_t o_number = number;

    while (number > 1 && factor < number && factor > 1){
        if (number % factor == 0){
            number /= factor;
            write_factor(slot, factor);
            printf("Thread [%d] number %d: Found factor %d\n", slot, o_number, factor);
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
    pthread_t threads[MAX_BITSIZE];

    for (int i = 0; i < MAX_BITSIZE; i++){
        struct thread_args *f_args = malloc(sizeof(struct thread_args));

        f_args->number = number;
        f_args->slot = slot;
        f_args->factor = number >> i;

        if (pthread_create(&threads[i], NULL, factorise, f_args)){
            error("thread create error", "factorise thread");
        }

        if (pthread_join(threads[i], NULL)){
            free(f_args);
            error("thread join error", "thread");
        }

        free(f_args);
    }

    notify_request_complete(slot);
    while (read_server_slot(slot) != 0) usleep(10);
    unassign_slot(slot);
    return NULL;
}


void cleanup() {
    printf("Destroying Mutexes... ");
    for (int i = 0; i < MAX_QUERIES; i++)
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
}

int main(int argc, char **argv){
    pthread_t threads[MAX_QUERIES];
    printf("Starting Server\n");
    printf("Initialising Shared memory...");
    initiliseShm();
    printf("complete\n");
    printf("Server ready\n");

    pthread_mutex_init(&(ShmPtr->client_mutex), NULL);
    for (int i = 0; i < MAX_QUERIES; i++){
        pthread_mutex_init(&(ShmPtr->server_mutex[i]), NULL);
    }

    signal(SIGINT, INTHandler);

    while (1){
        while (read_client_flag() == 0) usleep(10);

        uint32_t number, slot;

        number = read_client_number();
        slot = assign_slot(number);

        struct thread_args *args = malloc(sizeof(struct thread_args));
        args->number = number;
        args->slot = slot;

        if (pthread_create(&threads[slot], NULL, factorsParentThread, args))
            error("thread create error", "factorsParentThread");
    }
    cleanup();
}