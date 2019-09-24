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
#include "shmdef.h"
#include <signal.h>

void INTHandler(int);
void cleanup_shm(void);
void listen(struct Memory *);

struct Memory *ShmPtr;
int ShmID;

void INTHandler(int sig){
    printf("\rReceived Interrupt signal, deleting shared memory\n");
    cleanup_shm();
    exit(0);
}

void cleanup_shm(){
    printf("Detaching shared memory ...");
    shmdt((void *)ShmPtr);
    printf(" complete\n");
    printf("Deleting shared memory  ...");
    shmctl(ShmID, IPC_RMID, NULL);
    printf(" complete\n");
}

void listen(struct Memory *ShmPtr){
    while (1){
        while (ShmPtr->clientflag == 0);

        uint32_t number = ShmPtr->number;
        ShmPtr->clientflag = 0;

        int slot;
        for (int i = 0; i < 10; i++){
            if (ShmPtr->slots[i] == 0){
                slot = i;
                break;
            }
        }
        printf("Server: Got %d from client\n", number);
    }
}

int main(int argc, char **argv){
    key_t ShmKey;
    
    signal(SIGINT, INTHandler);


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
    


    listen(ShmPtr);
}