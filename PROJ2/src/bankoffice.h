#include <sope.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#include "logfileopeners.h"
#include "queue.h"
#include "fifoMaker.h"
#include "replymaker.h"

void* runBankOffice();
void createBankOffices(int numOffices);
void destroyBankOffices();

void addRequestToQueue(tlv_request_t request);
tlv_request_t getRequestFromQueue(int threadID);

void sendReply(tlv_reply_t reply, char* userFifoPath, int threadID);

void assignShutdownFD(int fd);
