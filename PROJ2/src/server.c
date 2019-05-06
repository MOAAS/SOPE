#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "constants.h"
#include <math.h>
#include <pthread.h>
#include <stdlib.h>

void* BankOffice() { return NULL; }

pthread_t* CreateBankOffices(int numberOfThreads)
{
    numberOfThreads = min(numberOfThreads, MAX_BANK_OFFICES);
    numberOfThreads = max(numberOfThreads, 1);
    pthread_t* officeTids = (pthread_t*) malloc(numberOfThreads * sizeof(pthread_t));
    for(unsigned int i = 0; i < numberOfThreads; i++) {
        pthread_t tid;
        if(pthread_create(&tid, NULL, &BankOffice, NULL) != 0) {
            perror("Thread Creation error in BankOffices: ");
            exit(1);
        }
        officeTids[i] = tid;
    }
    return officeTids;
}

void CreateUser(int id, char* password)
{

}

int main(int argc, char* argv[])
{
    if(argc == 3)
    {
        int numberOfOffices = atoi(argv[1]);
        CreateBankOffices(numberOfOffices);
        CreateUser(ADMIN_ACCOUNT_ID, argv[2]);
    }
    else
    {
        printf("INVALID ARGUMENTS!\n");
        exit(1);
    }
    
    printf("SERVER!\n");

    // Creating FIFO
    char *secure_srv = SERVER_FIFO_PATH;
    mkfifo(secure_srv, 0777);
    int server_fd = open(secure_srv, O_RDONLY);

    close(server_fd);

    return 0;
}