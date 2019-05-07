#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "types.h"
#include "sope.h"
#include "constants.h"

char* makeUserFifo();
void deleteUserFifo();

void makeServerFifo();
int openServerFifo(int mode);

char* getUserFifoPath(int pid);
