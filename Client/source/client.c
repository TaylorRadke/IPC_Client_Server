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

void error(char *err, char *source){
    printf("*** %s ( Client - %s) %s ***\n", err, source, strerror(errno));
    exit(1);
}

int processing(){
    int p = 0;
    for (int i = 0; i < 10; i++){
        pthread_mutex_lock(&(ShmPtr->server_mutex[i]));
        p += ShmPtr->assigned[i];
        pthread_mutex_unlock(&(ShmPtr->server_mutex[i]));
    }
    printf("Currently %d processing\n", p);
    return p;
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

      // processing();
        while (ShmPtr->clientflag != 0);
        pthread_mutex_lock(&(ShmPtr->client_mutex));
        ShmPtr->number = atoi(input);
        ShmPtr->clientflag = 1;
        pthread_mutex_unlock(&(ShmPtr->client_mutex));
        printf("Sent input\n");
        // if (1 < 10){
        //     while (ShmPtr->clientflag != 0);
        //     pthread_mutex_lock(&(ShmPtr->client_mutex));
        //     ShmPtr->number = atoi(input);
        //     ShmPtr->clientflag = 1;
        //     pthread_mutex_unlock(&(ShmPtr->client_mutex));
        // } else {
        //     printf("Warning: Server is currently busy at this time\n");
        // }
    }
}

void *watchServerFlags(void *args){
    while (1) {
        for (int i = 0; i < 10; i++){
            if (ShmPtr->serverflag[i] == 1){
                pthread_mutex_lock(&(ShmPtr->server_mutex[i]));
                printf("\rClient: Server got factor %d for input %d\n", ShmPtr->slots[i], ShmPtr->numbers[i]);
                ShmPtr->serverflag[i] = 0;
                pthread_mutex_unlock(&(ShmPtr->server_mutex[i]));
            }
        }
    }
}

void *watchServerRequestComplete(void *args){
    while (1){
        while (ShmPtr->request_complete == -1);
        pthread_mutex_lock(&(ShmPtr->client_mutex));
        printf("Client: Server finished request %d for input %d\n", ShmPtr->request_complete, ShmPtr->numbers[ShmPtr->request_complete]);
        ShmPtr->request_complete = -1;
        pthread_mutex_unlock(&(ShmPtr->client_mutex));
    }
}

int main(int argc, char **argv){
    key_t ShmKey;
    int ShmID;
    pthread_t input_thread, serverflag_thread, requestcomplete_thread;

    ShmKey = ftok("../../", 'x');
    
    ShmID = shmget(ShmKey, sizeof(struct Memory), 0666);
    if (ShmID < 0)
        error("shmget error", "shmget");
    
    ShmPtr = (struct Memory *) shmat(ShmID, NULL, 0);
    if ((int) ShmPtr == -1)
        error("shmat error", "shmat");

    if (pthread_create(&input_thread, NULL, listenStdin, NULL))
        error("thread create error", "input thread");
    

    if (pthread_create(&serverflag_thread, NULL, watchServerFlags, NULL))
        error("thread create error", "watchServerFlags thread");


    if (pthread_create(&requestcomplete_thread, NULL, watchServerRequestComplete, NULL))
        error("thread create error", "watchRequestComplete thread");

    if (pthread_join(input_thread, NULL))
        error("thread join error", "input thread");

    if (pthread_join(serverflag_thread, NULL))
        error("thread join error", "serverflag thread");

    if (pthread_join(requestcomplete_thread, NULL))
        error("thread join error", "requestcomplete thread");

    printf("Client has detached from shared memory\n");
    printf("Client exits ...\n");
    exit(0);
}