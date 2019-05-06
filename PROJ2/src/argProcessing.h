#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sope.h"

typedef struct {
    int numOffices;
    char* adminPassword;
} ServerArgs;

typedef struct {
    int id;
    char* password;
    int delayMS;
    int opcode;
    char* args;
} UserArgs;

ServerArgs processServerArgs(int argc, char* argv[]);
UserArgs processUserArgs(int argc, char* argv[]);
