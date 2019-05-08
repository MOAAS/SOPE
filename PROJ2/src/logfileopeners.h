#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "sope.h"

void openSLog();
void openULog();

int getSLogFD();
int getULogFD();

void logErrorReply(int fd, int id, int retCode, tlv_request_t request);
void logShutdownReply(int fd, int id, int accountId, int activeOffices);
