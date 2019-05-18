#include "logfileopeners.h"

static int ulogFD = 0;
static int slogFD = 0;

void openSLog() {
    slogFD = open("slog.txt", O_WRONLY | O_CREAT | O_APPEND);
    if (slogFD == -1) {
        perror("Opening server log file");
        exit(1);
    }
}

void openULog() {
    ulogFD = open("ulog.txt", O_WRONLY | O_CREAT | O_APPEND);
    if (ulogFD == -1) {
        perror("Opening user log file");
        exit(1);
    }
}

int getSLogFD() {
    return slogFD;
}

int getULogFD() {
    return ulogFD;
}
