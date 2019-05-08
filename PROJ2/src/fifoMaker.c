#include "fifoMaker.h"

static char* userFifoPath = NULL;

char* makeUserFifo() {
    userFifoPath = getUserFifoPath(getpid());
    if (mkfifo(userFifoPath, 0777) == -1) {
        perror("User fifo creation");
        exit(1);
    }
    return userFifoPath;
}

void deleteUserFifo() {
    if (userFifoPath == NULL)
        return;
    unlink(userFifoPath);
    free(userFifoPath);
    userFifoPath = NULL;
}

void makeServerFifo() {
    unlink(SERVER_FIFO_PATH);
    if (mkfifo(SERVER_FIFO_PATH, 0777) == -1) {
        perror("Server fifo creation");
        exit(1);
    }
}

int openServerFifo(int mode) {
    int serverFifoFD = open(SERVER_FIFO_PATH, mode);
    if (serverFifoFD == -1) {
        perror("Server fifo opening");
        exit(1);
    }
    return serverFifoFD;
}

char* getUserFifoPath(int pid) {
    char* userFifoPath = malloc(USER_FIFO_PATH_LEN + 1);
    char userPID[WIDTH_ID + 1];
    sprintf(userPID, "%05d", pid);
    strcpy(userFifoPath, USER_FIFO_PATH_PREFIX);
    strcat(userFifoPath, userPID);
    return userFifoPath;
}
