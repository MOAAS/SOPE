#include <sope.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#include "logfileopeners.h"
#include "queue.h"
#include "fifomaker.h"
#include "replymaker.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b)) 

void* runBankOffice();
void createBankOffices(int numOffices, int serverFifoFDW);
void destroyBankOffices();

void addRequestToQueue(tlv_request_t request);
tlv_request_t getRequestFromQueue(int threadID);

void sendReply(tlv_reply_t reply, char* userFifoPath, int threadID);
