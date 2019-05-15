#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>

#include "logfileopeners.h"
#include "fifoMaker.h"
#include "argProcessing.h"
#include "sope.h"
#include "accounts.h"
#include "bankoffice.h"

void readRequests(int serverFifoFD);
void addRequestToQueue(tlv_request_t request);

void setupAccounts(char* adminPassword) {
    clearAccounts();
    lockAccount(0, 0);
    createAccount(ADMIN_ACCOUNT_ID, 0, adminPassword, 0);
    unlockAccount(0, 0);
}

int main(int argc, char* argv[]) {    
    ServerArgs args = processServerArgs(argc, argv);
    openSLog();


    setupAccounts(args.adminPassword);
    createBankOffices(args.numOffices);

    makeServerFifo();
    int serverFifoFDR = openServerFifo(O_RDONLY);
    int serverFifoFDW = openServerFifo(O_WRONLY);
    assignShutdownFD(serverFifoFDW);

    readRequests(serverFifoFDR);

    destroyBankOffices();
    destroyAccounts();

    close(serverFifoFDR);
    unlink(SERVER_FIFO_PATH);
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
    }
}
