#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "shmdef.h"

int main(int argc, char **argv){
    key_t ShmKey;
    int ShmID;
    struct Memory *ShmPtr;

    ShmKey = ftok("../../", 'x');
    ShmID = shmget(ShmKey, sizeof(struct Memory), 0666);

    if (ShmID < 0){
        printf("*** shmget error (client) ***\n");
        exit(1);
    }

    ShmPtr = (struct Memory *) shmat(ShmID, NULL, 0);

    if ((int) ShmPtr == -1){
        printf("*** shmat error (client) ***\n");
        exit(1);
    }

    while (1){

    }


    shmdt((void *)ShmPtr);
    printf("Client has detached from shared memory\n");
    printf("Client exits ...\n");
    exit(0);


}