#include <stdlib.h>
#include <string.h>
#include "sope.h"

void clearAccounts();
bank_account_t createAccount(uint32_t id, uint32_t balance, char* password);

char* generateSalt();
char* generateHash(char* password, char* salt);
