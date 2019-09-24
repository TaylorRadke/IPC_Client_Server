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


void listen(struct Memory *ShmPtr){
    struct pollfd stdinfd;
    stdinfd.events = POLLIN;
    stdinfd.fd = 0;

    while (1){
        char input[100];
        scanf("%s", input);

        if (ShmPtr->clientflag != 0){
            while (ShmPtr->clientflag != 0);
        }
        ShmPtr->number = atoi(input);
        ShmPtr->clientflag = 1;
    }
}

int main(int argc, char **argv){
    key_t ShmKey;
    struct Memory *ShmPtr;
    int ShmID;
    
    ShmKey = ftok("../../", 'x');
    
    ShmID = shmget(ShmKey, sizeof(struct Memory), 0666);
    if (ShmID < 0){
        printf("*** shmget error (server) : %s ***\n", strerror(errno));
        exit(1);
    }
    ShmPtr = (struct Memory *) shmat(ShmID, NULL, 0);
    if ((int) ShmPtr == -1){
        printf("*** shmat error (server) ***\n");
        exit(1);
    }

    listen(ShmPtr);

    printf("Client has detached from shared memory\n");
    printf("Client exits ...\n");
    exit(0);


}