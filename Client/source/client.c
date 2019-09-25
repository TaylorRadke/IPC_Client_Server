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

#include "shmdef.h"

struct Memory *ShmPtr;

void error(char *msg){
    printf("*** %s (client) %s ***\n", msg, strerror(errno));
    exit(1);
}

void *listenStdin(void *args){
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

        if (ShmPtr->processing < 10){
            while (ShmPtr->clientflag == 1);
            pthread_mutex_lock(&(ShmPtr->client_mutex));
            ShmPtr->number = atoi(input);
            ShmPtr->clientflag = 1;
            pthread_mutex_unlock(&(ShmPtr->client_mutex));
        } else {
            printf("Warning: Server is currently busy at this time\n");
        }
    }
}

void *listenShm(void *args){
    while (1) {
        for (int i = 0; i < 10; i++){
            if (ShmPtr->serverflag[i] == 1){
                pthread_mutex_lock(&(ShmPtr->mutex_slots[i]));
                printf("\rClient: Server got factor %d for input %d\n", ShmPtr->slots[i], ShmPtr->numbers[i]);
                ShmPtr->serverflag[i] = 0;
                pthread_mutex_unlock(&(ShmPtr->mutex_slots[i]));
            }
        }
    }
}

int main(int argc, char **argv){
    key_t ShmKey;
    int ShmID;
    pthread_t input_thread, shm_thread;

    ShmKey = ftok("../../", 'x');
    
    ShmID = shmget(ShmKey, sizeof(struct Memory), 0666);
    if (ShmID < 0){
        error("shmget error");
    }
    ShmPtr = (struct Memory *) shmat(ShmID, NULL, 0);
    if ((int) ShmPtr == -1){
        error("shmat error");
    }

    if (pthread_create(&input_thread, NULL, listenStdin, NULL)){
        error("thread create error");
    }

    if (pthread_create(&shm_thread, NULL, listenShm, NULL)){
        error("thread create error");
    }

    if (pthread_join(input_thread, NULL)){
        error("thread join error");
    }

    if (pthread_join(shm_thread, NULL)){
        error("thread join error");
    }

    printf("Client has detached from shared memory\n");
    printf("Client exits ...\n");
    exit(0);


}