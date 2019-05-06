#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>

#include "fifoMaker.h"
#include "argProcessing.h"
#include "sope.h"
#include "accounts.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b)) 

void* runBankOffice();
pthread_t* createBankOffices(int numOffices);
void destroyBankOffices(pthread_t* offices, int numOffices);

void readRequests(int serverFifoFD, int serverFifoFDW);
void addRequestToQueue(tlv_request_t request);


int main(int argc, char* argv[]) {    
    ServerArgs args = processServerArgs(argc, argv);

    clearAccounts();
    createAccount(ADMIN_ACCOUNT_ID, 0, args.adminPassword);    
    pthread_t* bankOffices = createBankOffices(args.numOffices);    
    makeServerFifo();
    int serverFifoFDR = openServerFifo(O_RDONLY);
    int serverFifoFDW = openServerFifo(O_WRONLY);

    readRequests(serverFifoFDR, serverFifoFDW);

    close(serverFifoFDR);
    unlink(SERVER_FIFO_PATH);
    destroyBankOffices(bankOffices, args.numOffices);
    return 0;
}

void* runBankOffice() { 
    return NULL; 
}

pthread_t* createBankOffices(int numOffices) {
    numOffices = min(numOffices, MAX_BANK_OFFICES);
    numOffices = max(numOffices, 1);
    pthread_t* officeTids = (pthread_t*) malloc(numOffices * sizeof(pthread_t));
    for(unsigned int i = 0; i < numOffices; i++) {
        pthread_t tid;
        if(pthread_create(&tid, NULL, &runBankOffice, NULL) != 0) {
            perror("Thread Creation error in BankOffices: ");
            exit(1);
        }
        officeTids[i] = tid;
    }
    return officeTids;
}

void destroyBankOffices(pthread_t* offices, int numOffices) {
    for (int i = 0; i < numOffices; i++) {
        pthread_join(offices[i], NULL);
    }
}

void readRequests(int serverFifoFD, int serverFifoFDW) {
    int numBytesRead;
    while (true) {
        tlv_request_t request;
        if ((numBytesRead = read(serverFifoFD, &request.type, sizeof(op_type_t))) == -1) {
            perror("Reading request type");
            return;
        }

        if ((numBytesRead = read(serverFifoFD, &request.length, sizeof(uint32_t))) == -1) {
            perror("Reading request length");
            return;
        }

        if ((numBytesRead = read(serverFifoFD, &request.value, request.length)) == -1) {
            perror("Reading request value");
            return;
        }

        if (numBytesRead == 0)
            break;

        // debug :D nao tirar pa nao encravare
        close(serverFifoFDW);

        addRequestToQueue(request);
    }
}

void addRequestToQueue(tlv_request_t request) {
    // DEBUG
    printf("Pid = %d | AccID = %d | Delay = %d | Pass = \"%s\"\n", request.value.header.pid, request.value.header.account_id, request.value.header.op_delay_ms, request.value.header.password);
    printf("Type = %d | Length = %d \n", request.type, request.length);
    if (request.type == OP_CREATE_ACCOUNT)
        printf("Create ACC: ID = %d | Bal = %d | Pass = \"%s\"\n", request.value.create.account_id, request.value.create.balance, request.value.create.password);
    if (request.type == OP_TRANSFER)
        printf("Transfer: ID = %d | amount = %d \n",  request.value.transfer.account_id, request.value.transfer.amount);
}
