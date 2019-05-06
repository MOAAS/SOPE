#include "accounts.h"

static bank_account_t accounts[MAX_BANK_ACCOUNTS];
static bool usedAccounts[MAX_BANK_ACCOUNTS];


void clearAccounts() {
    for (int i = 0; i < MAX_BANK_ACCOUNTS; i++) {
        usedAccounts[i] = false;
    }
}

bank_account_t createAccount(uint32_t id, uint32_t balance, char* password) {
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