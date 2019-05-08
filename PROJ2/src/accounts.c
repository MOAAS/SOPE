#include "accounts.h"

static pthread_mutex_t accountMutexes[MAX_BANK_ACCOUNTS];
static bank_account_t accounts[MAX_BANK_ACCOUNTS];
static bool usedAccounts[MAX_BANK_ACCOUNTS];

bank_account_t* getAccount(uint32_t account_id)
{
    if(!usedAccounts[account_id]) return NULL;
    else return &accounts[account_id];
}

void clearAccounts() {
    for (int i = 0; i < MAX_BANK_ACCOUNTS; i++) {
        usedAccounts[i] = false;
        pthread_mutex_init (&accountMutexes[i], NULL);
    }
}

void destroyAccounts() {
    for (int i = 0; i < MAX_BANK_ACCOUNTS; i++) {
        usedAccounts[i] = false;
        pthread_mutex_destroy(&accountMutexes[i]);
    }
}

void lockAccount(uint32_t id, int threadID) {
    logSyncMech(getSLogFD(), threadID, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_ACCOUNT, id);
    pthread_mutex_lock(&accountMutexes[id]);
}

void unlockAccount(uint32_t id, int threadID) {
    pthread_mutex_unlock(&accountMutexes[id]);
    logSyncMech(getSLogFD(), threadID, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_ACCOUNT, id);
}

void lockDoubleAccount(uint32_t id1, uint32_t id2, int threadID) {
    if (id1 == id2) {
        lockAccount(id1, threadID);
        return;
    }
    uint32_t first, second;
    first = min(id1, id2);
    second = max(id1, id2);
    lockAccount(first, threadID);
    lockAccount(second, threadID);
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

bank_account_t createAccount(uint32_t id, uint32_t balance, char* password, int threadNum) {
    char* salt = generateSalt();
    char* hash = generateHash(password, salt);
    bank_account_t account;
    account.account_id = id;
    account.balance = balance;
    strcpy(account.salt, salt);
    strcpy(account.hash, hash);
    free(salt);
    free(hash);

    accounts[id] = account;
    usedAccounts[id] = true;

    logAccountCreation(getSLogFD(), threadNum, &account);
    return account;
}

char* generateSalt() {
    srand(100);
    char* salt = malloc(SALT_LEN + 1);
    for(int i = 0; i < SALT_LEN; i++) {
        sprintf(salt + i, "%x", rand() % 16);
    }
    return salt;
}

char* generateHash(char* password, char* salt) {
    char* hash = malloc(HASH_LEN + 1);
    char* command = malloc(1000);
    sprintf(command, "echo -n %s%s | sha256sum", password, salt);
    FILE* fp = popen(command ,"r");
    if (fp == NULL) {
        perror(command);
        exit(1);
    }
    fgets(hash, HASH_LEN + 1, fp);
    free(command);
    return hash;
}