#include <stdlib.h>
#include <string.h>

#include "sope.h"
#include "logfileopeners.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b)) 

void clearAccounts();
void destroyAccounts();

void createAccount(uint32_t id, uint32_t balance, char* password, int threadNum);

char* generateSalt();
char* generateHash(char* password, char* salt);
/**
 * @param id of the account we want to find
 * @ret Returns NULL if there's no such account, or the account if it exists
**/
bank_account_t* getAccount(uint32_t account_id);

void lockAccount(uint32_t id, int threadID);
void unlockAccount(uint32_t id, int threadID);
