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
    if(numberOfThreads > MAX_BANK_OFFICES) numberOfThreads = MAX_BANK_OFFICES;
    pthread_t* officeTids = (pthread_t*) malloc(numberOfThreads * sizeof(pthread_t));
    for(unsigned int i = 0; i < numberOfThreads; i++)
    {
        pthread_t tid;
        int err = pthread_create(&tid, NULL, &BankOffice, NULL);
        if(err != 0)
        {
            perror("Thread Creation error in BankOffices: ");
        }

        officeTids[i] = tid;
    }
    return officeTids;
}

void CreateUser(int id, char* password)
{

}

int main(int agrc, char* argv[])
{
    if(agrc == 3)
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