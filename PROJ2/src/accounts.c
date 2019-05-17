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

void createAccount(uint32_t id, uint32_t balance, char* password, int threadNum) {
    char* salt = generateSalt();
    char* hash = generateHash(password, salt);
    if (salt == NULL || hash == NULL)
        return;
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
}

char* generateSalt() {
    char* salt = malloc(SALT_LEN + 1);
    for(int i = 0; i < SALT_LEN; i++) {
        sprintf(salt + i, "%x", rand() % 16);
    }
    return salt;
}

char* generateHash(char* password, char* salt) {
    char* hash = malloc(HASH_LEN + 1);
    char* concatd = malloc(1000);
    sprintf(concatd, "%s%s", password, salt);


    int fd1[2], fd2[2];
    pipe(fd1);
    pipe(fd2);
    if(fork() == 0) {
        if (close(fd1[PIPE_READ]) == -1) {
            perror("Error closing coprocess FD1[READ]");
            exit(1);
        }
        if (close(fd2[PIPE_WRITE]) == -1) {
            perror("Error closing coprocess FD2[WRITE]");
            exit(1);
        }
        if (dup2(fd1[PIPE_WRITE], STDOUT_FILENO) == -1) {
            perror("Error invoking dup2 to STDOUT");
            exit(1);
        }
        if (dup2(fd2[PIPE_READ], STDIN_FILENO) == -1) {
            perror("Error invoking dup2 to STDIN");
            exit(1);
        }
        if (execlp("sha256sum", "sha256sum", NULL) == -1) {
            perror("Error invoking execlp");
            exit(1);
        }
        exit(0);
    } else {
        if (close(fd2[PIPE_READ]) == -1) {
            perror("Error closing process FD2[READ]");
            return NULL;
        }
        if (close(fd1[PIPE_WRITE]) == -1) {
            perror("Error closing process FD1[WRITE]");    
            return NULL;
        }    
        if (write(fd2[PIPE_WRITE], concatd, strlen(concatd)) != strlen(concatd)) {
            printf("Error writing to FD2[WRITE]\n");
            return NULL;
        }
        if (close(fd2[PIPE_WRITE]) == -1) {
            perror("Error closing process FD2[WRITE]");
            return NULL;
        }
        if (read(fd1[PIPE_READ], hash, HASH_LEN) != HASH_LEN) {
            printf("Error reading from FD1[READ]\n");
            return NULL;
        }
        if (close(fd1[PIPE_READ]) == -1) {
            perror("Error closing process FD1[READ]");
            return NULL;
        }
    }


    hash[HASH_LEN] = '\0';

    /*
    dup2(getSLogFD(), STDOUT_FILENO);
    printf("concatd: %s\n", concatd);
    printf("pass: %s\n", password);
    printf("hash: %s\n", hash);
    printf("salt: %s\n", salt);
    */

    return hash;
}