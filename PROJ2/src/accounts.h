#include <stdlib.h>
#include <string.h>
#include "sope.h"

void clearAccounts();
bank_account_t createAccount(uint32_t id, uint32_t balance, char* password);

char* generateSalt();
char* generateHash(char* password, char* salt);
/**
 * @param id of the account we want to find
 * @ret Returns NULL if there's no such account, or the account if it exists
**/
bank_account_t* getAccount(uint32_t account_id);