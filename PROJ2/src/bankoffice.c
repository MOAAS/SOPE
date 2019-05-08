#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bankoffice.h"
#include "accounts.h"
#include "queue.h"
#include "semaphore.h"

static const int BUFFER_SIZE = 5000;

static pthread_t* officeTids;
static int* threadNums;
static int numberOfOffices = 0;

static bool* activeOffices; // partilhadooooooooo

static pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
static RequestQueue* queue;
static sem_t queueEmptySem;
static sem_t queueFullSem;

static int serverFifoFD;

int getNumActiveOffices() {
    int numActiveOffices = 0;
    for (int i = 0; i < numberOfOffices; i++) {
        numActiveOffices += activeOffices[i];
    }
    return numActiveOffices;
}

void waitForRequestFinish() {
    bool queueEmpty = false;
    while(!queueEmpty) {
        pthread_mutex_lock(&queueMutex); // Lock
        queueEmpty = queue_empty(queue);
        pthread_mutex_unlock(&queueMutex); // Unlock
    }
    while (getNumActiveOffices() > 0);
}

void opDelay(int delayMS, int threadID) {
    usleep(delayMS * 1000);
    logDelay(getSLogFD(), threadID, delayMS);
}

bool validateAccount(tlv_request_t request, int threadID, char* userFifoPath)
{    
    bank_account_t* account = getAccount(request.value.header.account_id);
    
    if(account == NULL)
    {
        //sendReply(,userFifoPath); //UNKNOWN ACCOUNT ERROR
        return false;
    }

    char* hash = generateHash(request.value.header.password, account->salt);

    if(strcmp(hash, account->hash) != 0) {
        //sendReply(,userFifoPath); //INCORRECT PASSWORD ERROR
        return false;
    }

    return true;
}

void handleCreateAccountRequest(tlv_request_t request, int threadID, char* userFifoPath)
{
    opDelay(request.value.header.op_delay_ms, threadID);

    uint32_t id = request.value.create.account_id; 

    if(id != ADMIN_ACCOUNT_ID)
    {
        //sendReply(,userFifoPath); //NOT ADMIN ERROR
        return;
    }

    if(getAccount(id) != NULL)
    {
        //sendReply(,userFifoPath); //ACCOUNT ALREADY IN USE
        return;
    }

    uint32_t balance = request.value.create.balance;
    char* password = request.value.create.password;
    createAccount(id, balance, password, threadID);
    //sendReply(,userFifoPath); //ACCOUNT CREATED
}

void handleTransferRequest(tlv_request_t request, int threadID, char* userFifoPath)
{
    opDelay(request.value.header.op_delay_ms, threadID);
    bank_account_t* srcAccount = getAccount(request.value.header.account_id);
    bank_account_t* destAccount = getAccount(request.value.transfer.account_id);

    if(srcAccount->account_id == ADMIN_ACCOUNT_ID)
    {
        //sendReply(,userFifoPath); //ADMIN ERROR
        return;
    }

    if(srcAccount->account_id == request.value.transfer.account_id)
    {
        //sendReply(,userFifoPath); //SAME ID ERROR
        return;
    }

    if(destAccount == NULL)
    {
        //sendReply(,userFifoPath); //DEST ACCOUNT DOESN'T EXIST ERROR
        return;
    }

    // falta verificar se destino e admin

    if((srcAccount->balance - request.value.transfer.amount) < 0)
    {
        //sendReply(,userFifoPath); //NOT ENOUGH MONEY ERROR
        return;
    }

    if((destAccount->balance + request.value.transfer.amount) > MAX_BALANCE)
    {
        //sendReply(,userFifoPath); //TOO HIGH ERROR
        return;
    }
}

void handleShutdownRequest(tlv_request_t request, int threadID, char* userFifoPath) {
    tlv_reply_t reply;
    if (request.value.header.account_id == ADMIN_ACCOUNT_ID) {
        opDelay(request.value.header.op_delay_ms, threadID);
        fchmod(serverFifoFD, 0444);
        close(serverFifoFD);
        reply = makeShutdownReply( request.value.header.account_id, getNumActiveOffices());
    }
    else reply = makeErrorReply(OP_NALLOW, request);
    sendReply(reply, userFifoPath, threadID);
}

void manageRequest(tlv_request_t request, int threadID)
{
    char* userFifoPath = getUserFifoPath(request.value.header.pid);

    lockAccount(request.value.header.account_id, threadID);
    if(!validateAccount(request, threadID, userFifoPath)) return;
    unlockAccount(request.value.header.account_id, threadID);

    switch(request.type)
    {
        case OP_CREATE_ACCOUNT:
            lockAccount(request.value.header.account_id, threadID);
            handleCreateAccountRequest(request, threadID, userFifoPath);
            unlockAccount(request.value.header.account_id, threadID);
            break;
        case OP_BALANCE:
            lockAccount(request.value.header.account_id, threadID);
            //sendReply(,userFifoPath); //BALANCE
            unlockAccount(request.value.header.account_id, threadID);
            break;
        case OP_TRANSFER:
            lockDoubleAccount(request.value.header.account_id, request.value.transfer.account_id, threadID);
            handleTransferRequest(request, threadID, userFifoPath);
            unlockDoubleAccount(request.value.header.account_id, request.value.transfer.account_id, threadID);
            break;
        case OP_SHUTDOWN:
            handleShutdownRequest(request, threadID, userFifoPath);
            break;
    }
}

void* runBankOffice(void* officeNum) {
    int num = *(int*)officeNum;
    activeOffices[num - 1] = false;

    while(true) {
        tlv_request_t request = getRequestFromQueue(num);
        manageRequest(request, num);
        activeOffices[num - 1] = false;
    }

    return NULL; 
}

void createQueue() {
    queue = queue_create(BUFFER_SIZE);
    int semValue;
    
    if (sem_init(&queueEmptySem, 0, BUFFER_SIZE) == -1) {
        perror("Error creating empty sem");
        exit(1);
    }
    
    sem_getvalue(&queueEmptySem, &semValue);
    logSyncMechSem(getSLogFD(), 0, SYNC_OP_SEM_INIT, SYNC_ROLE_PRODUCER, 0, semValue);

    if (sem_init(&queueFullSem, 0, 0) == -1) {
        perror("Error creating full sem");
        exit(1);
    }
    sem_getvalue(&queueFullSem, &semValue);
    logSyncMechSem(getSLogFD(), 0, SYNC_OP_SEM_INIT, SYNC_ROLE_PRODUCER, 0, semValue); 
}

void destroyQueue() {
    if (sem_destroy(&queueEmptySem) == -1) {
        perror("Error destroying empty sem");
        exit(1);
    }

    if (sem_destroy(&queueFullSem) == -1) {
        perror("Error destroying full sem");
        exit(1);
    }
}

void createBankOffices(int numOffices, int serverFifoFDW) {
    serverFifoFD = serverFifoFDW;
    numOffices = min(numOffices, MAX_BANK_OFFICES);
    numOffices = max(numOffices, 1);
    numberOfOffices = numOffices;
    officeTids = malloc(numOffices * sizeof(pthread_t));
    threadNums = malloc(numOffices * sizeof(int));
    activeOffices = malloc(numOffices * sizeof(bool));
    createQueue();
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
    waitForRequestFinish();
    for (int i = 0; i < numberOfOffices; i++) {
        pthread_cancel(officeTids[i]);
        pthread_join(officeTids[i], NULL);
        logBankOfficeClose(getSLogFD(), i + 1, officeTids[i]);
    }
    free(officeTids);
    destroyQueue();
    numberOfOffices = 0;
}

// Produtor
void addRequestToQueue(tlv_request_t request) {
    int semValue;
    sem_getvalue(&queueEmptySem, &semValue);
    logSyncMechSem(getSLogFD(), 0, SYNC_OP_SEM_WAIT, SYNC_ROLE_PRODUCER, request.value.header.pid, semValue);

    sem_wait(&queueEmptySem); // Wait

    logSyncMech(getSLogFD(), 0, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_PRODUCER, request.value.header.pid);
    pthread_mutex_lock(&queueMutex); // Lock
    queue_push(queue, request); // Append
    pthread_mutex_unlock(&queueMutex); // Unlock
    logSyncMech(getSLogFD(), 0, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_PRODUCER, request.value.header.pid);

    sem_post(&queueFullSem); // Signal

    sem_getvalue(&queueFullSem, &semValue);
    logSyncMechSem(getSLogFD(), 0, SYNC_OP_SEM_POST, SYNC_ROLE_PRODUCER, request.value.header.pid, semValue);
    
    logRequest(getSLogFD(), 0, &request);

    // DEBUG
    printf("ADD REQUEST::::\n");
    printf("Pid = %d | AccID = %d | Delay = %d | Pass = \"%s\"\n", request.value.header.pid, request.value.header.account_id, request.value.header.op_delay_ms, request.value.header.password);
    printf("Type = %d | Length = %d \n", request.type, request.length);
    if (request.type == OP_CREATE_ACCOUNT)
        printf("Create ACC: ID = %d | Bal = %d | Pass = \"%s\"\n", request.value.create.account_id, request.value.create.balance, request.value.create.password);
    if (request.type == OP_TRANSFER)
        printf("Transfer: ID = %d | amount = %d \n",  request.value.transfer.account_id, request.value.transfer.amount);
}

// Consumidor
tlv_request_t getRequestFromQueue(int threadID) {
    int semValue;
    sem_getvalue(&queueFullSem, &semValue);
    logSyncMechSem(getSLogFD(), threadID, SYNC_OP_SEM_WAIT, SYNC_ROLE_CONSUMER, 0, semValue);

    sem_wait(&queueFullSem); // Wait

    activeOffices[threadID - 1] = true;
    
    logSyncMech(getSLogFD(), threadID, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_CONSUMER, 0);
    pthread_mutex_lock(&queueMutex); // Lock
    tlv_request_t request = queue_pop(queue); // Take
    pthread_mutex_unlock(&queueMutex); // Unlock
    logSyncMech(getSLogFD(), threadID, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_CONSUMER, request.value.header.pid);

    sem_post(&queueEmptySem); // Signal

    sem_getvalue(&queueEmptySem, &semValue);
    logSyncMechSem(getSLogFD(), threadID, SYNC_OP_SEM_POST, SYNC_ROLE_CONSUMER, request.value.header.pid, semValue);

    logRequest(getSLogFD(), threadID, &request);

    // DEBUG
    printf("GOT REQUEST::::\n");
    printf("Pid = %d | AccID = %d | Delay = %d | Pass = \"%s\"\n", request.value.header.pid, request.value.header.account_id, request.value.header.op_delay_ms, request.value.header.password);
    printf("Type = %d | Length = %d \n", request.type, request.length);
    if (request.type == OP_CREATE_ACCOUNT)
        printf("Create ACC: ID = %d | Bal = %d | Pass = \"%s\"\n", request.value.create.account_id, request.value.create.balance, request.value.create.password);
    if (request.type == OP_TRANSFER)
        printf("Transfer: ID = %d | amount = %d \n",  request.value.transfer.account_id, request.value.transfer.amount);

    
    return request;
}


void sendReply(tlv_reply_t reply, char* userFifoPath, int threadID) {
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
    logReply(getSLogFD(), threadID, &reply);
}
