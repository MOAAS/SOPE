#include "logfileopeners.h"

static int ulogFD = 0;
static int slogFD = 0;

void logEmptyReply(int fd, int id, int retCode, tlv_request_t request) {
    tlv_reply_t reply;
    reply.type = request.type;
    reply.value.header.account_id = request.value.header.account_id;
    reply.value.header.ret_code = retCode;
    reply.value.balance.balance = 0;
    reply.value.transfer.balance = 0;
    reply.value.shutdown.active_offices = 0;
    reply.length = sizeof(reply.value.header);
    logReply(fd, id, &reply);
}

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
