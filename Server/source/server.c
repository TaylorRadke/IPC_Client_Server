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
    ShmID = shmget(ShmKey, sizeof(struct Memory), IPC_CREAT | 0666);
    if (ShmID < 0){
        printf("*** shmat error (server) ***\n");
        exit(1);
    }
    ShmPtr = (struct Memory *) shmat(ShmID, NULL, 0);
    if ((int) ShmPtr == -1){
        printf("*** shmat error (server) ***\n");
        exit(1);
    }

    printf("Server has attached the shared memory\n");
    ShmPtr->status = NOT_READY;

    while (1){
        while (ShmPtr->status != FILLED) sleep (1);

        printf("Server: Received value %d from client. Sending back %d\n", ShmPtr->message, ShmPtr->message * 2);

        ShmPtr->status = TAKEN;
        ShmPtr->message = ShmPtr->message * 2;
        ShmPtr->status = FILLED;
    }

    printf("Server has detected completion of client\n");
    shmdt((void *)ShmPtr);
    printf("Server has detached from client\n");
    shmctl(ShmID, IPC_RMID, NULL);
    printf("Server has removed shared memory\n");
    printf("Server exits...\n");
    exit(0);
}