#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bankoffice.h"
#include "requestsQueue.h"
#include "accounts.h"

static pthread_t* officeTids;
static int* threadNums;
static int numberOfOffices = 0;

static bool* usedOffices; // partilhadooooooooo
static pthread_mutex_t mootex = PTHREAD_MUTEX_INITIALIZER; // test :D
static RequestsQueue* requestsQueue;

bool validateAccount(tlv_request_t request, char* userFifoPath, bank_account_t* account)
{
    uint32_t account_id = request.value.header.account_id;

    if((account = getAccount(account_id)) == NULL)
    {
        //sendReply(,userFifoPath); //UNKNOWN ACCOUNT ERROR
        return false;
    }

    char* hash = generateHash(request.value.header.password, account->salt);
    if(hash != account->hash)
    {
        //sendReply(,userFifoPath); //INCORRECT PASSWORD ERROR
        return false;
    }

    return true;
}

void handleCreateAccountRequest(tlv_request_t request, bank_account_t* account)
{
    if(account->account_id != ADMIN_ACCOUNT_ID)
    {
        //sendReply(,userFifoPath); //NOT ADMIN ERROR
        return;
    }

    uint32_t id = request.value.create.account_id; 
    if(getAccount(id) != NULL)
    {
        //sendReply(,userFifoPath); //ACCOUNT ALREADY IN USE
        return;
    }

    uint32_t balance = request.value.create.balance;
    char* password = request.value.create.password;
    createAccount(id, balance, password);
    //sendReply(,userFifoPath); //ACCOUNT CREATED
}

void handleBalanceRequest(tlv_request_t request, bank_account_t* account)
{

}

void manageRequest(tlv_request_t request)
{
    char pidStr[WIDTH_ID];
    sprintf(pidStr, "%d", request.value.header.pid);
    char* userFIFOpath = (char*) malloc(USER_FIFO_PATH_LEN);
    userFIFOpath = strcat(USER_FIFO_PATH_PREFIX, pidStr);

    bank_account_t* account;
    if(!validateAccount(request, userFIFOpath, account)) return;

    switch(request.type)
    {
        case OP_CREATE_ACCOUNT:
            handleCreateAccountRequest(request, account);
            break;
        case OP_BALANCE:
            break;
        case OP_TRANSFER:
            break;
        case OP_SHUTDOWN:
            break;
        case __OP_MAX_NUMBER:
            break;
    }
}

void* runBankOffice(void* officeNum) {
    int num = *(int*)officeNum;
    usedOffices[num - 1] = true;

    while(true)
    {
        tlv_request_t* request = getFront(requestsQueue);
        if(request == NULL) continue;

        popFront(requestsQueue);
        manageRequest(*request);
    }

    usedOffices[num - 1] = false;
    return NULL; 
}

void createBankOffices(int numOffices) {
    numOffices = min(numOffices, MAX_BANK_OFFICES);
    numOffices = max(numOffices, 1);
    numberOfOffices = numOffices;
    officeTids = malloc(numOffices * sizeof(pthread_t));
    threadNums = malloc(numOffices * sizeof(int));
    usedOffices = malloc(numOffices * sizeof(bool));
    requestsQueue = createRequestsQueue();
    for(int i = 0; i < numOffices; i++) {
        pthread_t tid;
        threadNums[i] = i + 1;
        if(pthread_create(&tid, NULL, &runBankOffice, &(threadNums[i])) != 0) {
            perror("Thread Creation error in BankOffices: ");
            exit(1);
        }
        logBankOfficeOpen(getSLogFD(), threadNums[i], tid);
        officeTids[i] = tid;
    }
}

void destroyBankOffices() {
    for (int i = 0; i < numberOfOffices; i++) {
        pthread_join(officeTids[i], NULL);
        logBankOfficeClose(getSLogFD(), i + 1, officeTids[i]);
    }
    free(officeTids);
    numberOfOffices = 0;
}

void addRequestToQueue(tlv_request_t request) {
    push(requestsQueue, request);

    // DEBUG
    printf("Pid = %d | AccID = %d | Delay = %d | Pass = \"%s\"\n", request.value.header.pid, request.value.header.account_id, request.value.header.op_delay_ms, request.value.header.password);
    printf("Type = %d | Length = %d \n", request.type, request.length);
    if (request.type == OP_CREATE_ACCOUNT)
        printf("Create ACC: ID = %d | Bal = %d | Pass = \"%s\"\n", request.value.create.account_id, request.value.create.balance, request.value.create.password);
    if (request.type == OP_TRANSFER)
        printf("Transfer: ID = %d | amount = %d \n",  request.value.transfer.account_id, request.value.transfer.amount);
}

void sendReply(tlv_reply_t reply, char* userFifoPath) {
    int userFifoFD = open(userFifoPath, O_WRONLY | O_NONBLOCK);
    if (userFifoFD == -1) {
        perror("Opening user Fifo");
        reply.value.header.ret_code = USR_DOWN;
        return;
    }
    if (write(userFifoFD, &reply, sizeof(op_type_t) + sizeof(uint32_t) + reply.length) == -1) {
        perror("Sending reply");
        exit(1);
    }
    close(userFifoFD);
    logReply(getSLogFD(), pthread_self(), &reply);
}