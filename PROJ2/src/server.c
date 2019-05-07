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

void readRequests(int serverFifoFD, int serverFifoFDW);
void addRequestToQueue(tlv_request_t request);

int main(int argc, char* argv[]) {    
    ServerArgs args = processServerArgs(argc, argv);
    openSLog();

    clearAccounts();
    bank_account_t admin_acc = createAccount(ADMIN_ACCOUNT_ID, 0, args.adminPassword);
    logAccountCreation(getSLogFD(), 0, &admin_acc);
    createBankOffices(args.numOffices);    

    makeServerFifo();
    int serverFifoFDR = openServerFifo(O_RDONLY);
    int serverFifoFDW = openServerFifo(O_WRONLY);

    readRequests(serverFifoFDR, serverFifoFDW);

    close(serverFifoFDR);
    unlink(SERVER_FIFO_PATH);
    destroyBankOffices();
    return 0;
}

void readRequests(int serverFifoFD, int serverFifoFDW) {
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

        // debug :D nao tirar pa nao encravare
        //close(serverFifoFDW);

        addRequestToQueue(request);
    }
}
