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
