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

static bool* activeOffices;

static pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
static RequestQueue* queue;
static sem_t queueEmptySem;
static sem_t queueFullSem;

static int shutdownFD;

void sendReply(tlv_reply_t reply, char* userFifoPath, int threadID);

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

void opDelayShutdown(int delayMS, int threadID) {
    logDelay(getSLogFD(), threadID, delayMS);
    usleep(delayMS * 1000);
}

void opDelay(int delayMS, int accID, int threadID) {
    logSyncDelay(getSLogFD(), threadID, accID, delayMS);
    usleep(delayMS * 1000);
}

void getSingleAccountAccess(uint32_t id, int delayMS, int threadID) {
    lockAccount(id, threadID);
    opDelay(delayMS, id, threadID);
}

void getDoubleAccountAccess(uint32_t id1, uint32_t id2, int delayMS, int threadID) {
    if (id1 == id2) {
        lockAccount(id1, threadID);
        opDelay(delayMS, id1, threadID);
        return;
    }
    uint32_t first, second;
    first = min(id1, id2);
    second = max(id1, id2);
    lockAccount(first, threadID);
    opDelay(delayMS, first, threadID);
    lockAccount(second, threadID);
    opDelay(delayMS, second, threadID);
}

void unlockDoubleAccount(uint32_t id1, uint32_t id2, int threadID) {
    if (id1 == id2) {
        unlockAccount(id1, threadID);
        return;
    }
    uint32_t first, second;
    first = max(id1, id2);
    second = min(id1, id2);
    unlockAccount(first, threadID);
    unlockAccount(second, threadID);
}

bool validateAccount(tlv_request_t request, tlv_reply_t* reply)
{
    uint32_t account_id = request.value.header.account_id;   
    bank_account_t* account = getAccount(account_id);
    
    if(account == NULL)
    {
        *reply = makeErrorReply(ID_NOT_FOUND, request);
        return false;
    }

    char* hash = generateHash(request.value.header.password, account->salt);

    if(strcmp(hash, account->hash) != 0) {
        *reply = makeErrorReply(LOGIN_FAIL, request);
        free(hash);
        return false;
    }

    free(hash);
    return true;
}

tlv_reply_t handleCreateAccountRequest(tlv_request_t request, int threadID)
{
    tlv_reply_t reply;
    uint32_t accountID = request.value.header.account_id;
    uint32_t newAccId = request.value.create.account_id;

    if(accountID != ADMIN_ACCOUNT_ID)
    {
        reply = makeErrorReply(OP_NALLOW, request);
        return reply;
    }

    if(getAccount(newAccId) != NULL)
    {
        reply = makeErrorReply(ID_IN_USE, request);
        return reply;
    }

    uint32_t balance = request.value.create.balance;
    char* password = request.value.create.password;
    createAccount(newAccId, balance, password, threadID);
    
    reply = makeCreateReply(accountID);

    return reply;
}

tlv_reply_t handleTransferRequest(tlv_request_t request, int threadID)
{
    tlv_reply_t reply;
    bank_account_t* srcAccount = getAccount(request.value.header.account_id);
    bank_account_t* destAccount = getAccount(request.value.transfer.account_id);

    if(srcAccount->account_id == ADMIN_ACCOUNT_ID)
    {
        reply = makeTransferReply(srcAccount->account_id, srcAccount->balance, OP_NALLOW);
        return reply;
    }

    if(destAccount == NULL)
    {
        reply = makeTransferReply(srcAccount->account_id, srcAccount->balance, ID_NOT_FOUND);
        return reply;
    }

    if (destAccount->account_id == ADMIN_ACCOUNT_ID)
    {
        reply = makeTransferReply(srcAccount->account_id, srcAccount->balance, OP_NALLOW);
        return reply;
    }

    if(srcAccount->account_id == request.value.transfer.account_id)
    {
        reply = makeTransferReply(srcAccount->account_id, srcAccount->balance, SAME_ID);
        return reply;
    }
    
    if(srcAccount->balance < request.value.transfer.amount)
    {
        reply = makeTransferReply(srcAccount->account_id, srcAccount->balance, NO_FUNDS);
        return reply;
    }

    if((destAccount->balance + request.value.transfer.amount) > MAX_BALANCE)
    {
        reply = makeTransferReply(srcAccount->account_id, srcAccount->balance, TOO_HIGH);
        return reply;
    }

    srcAccount->balance -= request.value.transfer.amount;
    destAccount->balance += request.value.transfer.amount;

    reply = makeTransferReply(srcAccount->account_id, srcAccount->balance, OK);

    return reply;
}

tlv_reply_t handleShutdownRequest(tlv_request_t request, int threadID) {
    opDelayShutdown(request.value.header.op_delay_ms, threadID);
    if (request.value.header.account_id == ADMIN_ACCOUNT_ID) {
        fchmod(shutdownFD, 0444);
        close(shutdownFD);
        return makeShutdownReply(request.value.header.account_id, getNumActiveOffices());
    }
    else return makeErrorReply(OP_NALLOW, request);
}

tlv_reply_t handleBalanceRequest(tlv_request_t request, int threadID)
{
    if (request.value.header.account_id != ADMIN_ACCOUNT_ID) {
        bank_account_t* account = getAccount(request.value.header.account_id);
        return makeBalanceReply(account->account_id, account->balance);
    }
    else return makeErrorReply(OP_NALLOW, request);
}

void manageRequest(tlv_request_t request, int threadID)
{
    char* userFifoPath = getUserFifoPath(request.value.header.pid);

    lockAccount(request.value.header.account_id, threadID);
    tlv_reply_t reply;
    bool validAccount = validateAccount(request, &reply);
    unlockAccount(request.value.header.account_id, threadID);
    
    if (!validAccount) {
        sendReply(reply, userFifoPath, threadID);
        return;
    }

    switch(request.type)
    {
        case OP_CREATE_ACCOUNT:
            getSingleAccountAccess(request.value.create.account_id, request.value.header.op_delay_ms, threadID);
            reply = handleCreateAccountRequest(request, threadID);
            unlockAccount(request.value.create.account_id, threadID);
            break;
        case OP_BALANCE:
            getSingleAccountAccess(request.value.header.account_id, request.value.header.op_delay_ms, threadID);
            reply = handleBalanceRequest(request, threadID);
            unlockAccount(request.value.header.account_id, threadID);
            break;
        case OP_TRANSFER:
            getDoubleAccountAccess(request.value.header.account_id, request.value.transfer.account_id, request.value.header.op_delay_ms, threadID);
            reply = handleTransferRequest(request, threadID);
            unlockDoubleAccount(request.value.header.account_id, request.value.transfer.account_id, threadID);
            break;
        case OP_SHUTDOWN:
            reply = handleShutdownRequest(request, threadID);
            break;
        case __OP_MAX_NUMBER: break;
    }

    sendReply(reply ,userFifoPath, threadID);
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

void assignShutdownFD(int fd) {
    shutdownFD = fd;
}

void createBankOffices(int numOffices) {
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
    logRequest(getSLogFD(), 0, &request);

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
        
    return request;
}


void sendReply(tlv_reply_t reply, char* userFifoPath, int threadID) {
    int userFifoFD = open(userFifoPath, O_WRONLY | O_NONBLOCK);
    if (userFifoFD == -1) {
        perror("Opening user Fifo");
        reply.value.header.ret_code = USR_DOWN;
    }
    else if (write(userFifoFD, &reply, sizeof(op_type_t) + sizeof(uint32_t) + reply.length) == -1) {
        perror("Sending reply");
        exit(1);
    }
    close(userFifoFD);
    logReply(getSLogFD(), threadID, &reply);
}
