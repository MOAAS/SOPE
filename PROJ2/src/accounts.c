#include "accounts.h"

#define PIPE_READ 0
#define PIPE_WRITE 1

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

    int fd1[2], fd2[2];
    pipe(fd1);
    pipe(fd2);
    if(fork() == 0) {
        close(fd1[PIPE_READ]);
        close(fd2[PIPE_WRITE]);
        dup2(fd1[PIPE_WRITE], STDOUT_FILENO);
        dup2(fd2[PIPE_READ], STDIN_FILENO);
        execlp("sha256sum", "sha256sum", NULL);
        exit(0);
    } else {
        close(fd2[PIPE_READ]);
        close(fd1[PIPE_WRITE]);
        write(fd2[PIPE_WRITE], strcat(password, salt), HASH_LEN);
        close(fd2[PIPE_WRITE]);
        read(fd1[PIPE_READ], hash, HASH_LEN);
        close(fd1[PIPE_READ]);
    }

    return hash;
}