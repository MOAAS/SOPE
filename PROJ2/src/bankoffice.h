#include <sope.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "logfileopeners.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b)) 

void* runBankOffice();
void createBankOffices(int numOffices);
void destroyBankOffices();

void addRequestToQueue(tlv_request_t request);
