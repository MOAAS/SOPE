#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include "logfileopeners.h"
#include "fifoMaker.h"
#include "argProcessing.h"
#include "sope.h"
#include "accounts.h"
#include "bankoffice.h"

void readRequests(int serverFifoFD);
void addRequestToQueue(tlv_request_t request);

void displayRequest(tlv_request_t request) {
    switch (request.type) {
        case OP_CREATE_ACCOUNT: printf("Got create account request from PID %d\n", request.value.header.pid); break;
        case OP_BALANCE: printf("Got balance request from PID %d\n", request.value.header.pid); break;
        case OP_TRANSFER: printf("Got transfer request from PID %d\n", request.value.header.pid); break;
        case OP_SHUTDOWN: printf("Got shutdown request from PID %d\n", request.value.header.pid); break;
        case __OP_MAX_NUMBER: break;
    }
}

void setupAccounts(char* adminPassword) {
    srand(time(NULL));
    clearAccounts();
    lockAccount(0, 0);
    createAccount(ADMIN_ACCOUNT_ID, 0, adminPassword, 0);
    unlockAccount(0, 0);
    printf("Created admin account.\n");
}


int main(int argc, char* argv[]) {    
    ServerArgs args = processServerArgs(argc, argv);
    openSLog();


    setupAccounts(args.adminPassword);
    createBankOffices(args.numOffices);
    printf("%d bank offices created.\n", args.numOffices);

    makeServerFifo();
    int serverFifoFDR = openServerFifo(O_RDONLY);
    int serverFifoFDW = openServerFifo(O_WRONLY);
    assignShutdownFD(serverFifoFDW);
    printf("Server fifo set up\n");

    readRequests(serverFifoFDR);

    destroyBankOffices();
    printf("%d bank offices destroyed.\n", args.numOffices);

    destroyAccounts();    
    printf("Destroyed all accounts.\n");    
    
    close(serverFifoFDR);
    unlink(SERVER_FIFO_PATH);
    printf("Server fifo shut down\n");
    return 0;
}

void readRequests(int serverFifoFD) {
    int numBytesRead;
    while (true) {
        tlv_request_t request;
        
        if ((numBytesRead = read(serverFifoFD, &request, sizeof(op_type_t) + sizeof(uint32_t))) == -1) {
            perror("Reading request type length");
            return;
        }

        if ((numBytesRead = read(serverFifoFD, &request.value, request.length)) == -1) {
            perror("Reading request value");
            return;
        }

        if (numBytesRead == 0)
            break;

        addRequestToQueue(request);
        displayRequest(request);
    }
}
