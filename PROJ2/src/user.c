#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#include "fifoMaker.h"
#include "argProcessing.h"
#include "sope.h"

tlv_request_t createRequest(UserArgs args);
req_header_t makeReqHeader(UserArgs args);
req_create_account_t makeCreateAccReq(char* args);
req_transfer_t makeTransferReq(char* args);

void sendRequest(tlv_request_t request, char* serverFifoPath);
tlv_reply_t awaitReply(char* userFifoPath);

void sigAlarmHandler();
void enableTimeoutAlarm();

int main(int argc, char * argv[]) {
    UserArgs args = processUserArgs(argc, argv);
    tlv_request_t request = createRequest(args);
    
    char* userFifoPath = makeUserFifo();
    char* serverFifoPath = SERVER_FIFO_PATH;

    sendRequest(request, serverFifoPath);

    enableTimeoutAlarm();

    tlv_reply_t reply = awaitReply(userFifoPath);

    deleteUserFifo();

    // Debug :D

    printf("Pid = %d | AccID = %d | Delay = %d | Pass = \"%s\"\n", request.value.header.pid, request.value.header.account_id, request.value.header.op_delay_ms, request.value.header.password);
    printf("Type = %d | Length = %d \n", request.type, request.length);
    if (args.opcode == OP_CREATE_ACCOUNT)
        printf("Create ACC: ID = %d | Bal = %d | Pass = \"%s\"\n", request.value.create.account_id, request.value.create.balance, request.value.create.password);
    if (args.opcode == OP_TRANSFER)
        printf("Transfer: ID = %d | amount = %d \n",  request.value.transfer.account_id, request.value.transfer.amount);

}

void sigAlarmHandler() {
    deleteUserFifo();
    exit(0);
}

void enableTimeoutAlarm() {
    struct sigaction sa;
    sa.sa_handler = sigAlarmHandler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);
    alarm(FIFO_TIMEOUT_SECS);
}

void sendRequest(tlv_request_t request, char* serverFifoPath) {
    int serverFifoFD = open(serverFifoPath, O_WRONLY | O_NONBLOCK);
    if (serverFifoFD == -1) {
        perror("Opening Server Fifo");
        exit(1);
    }
    if (write(serverFifoFD, &request, sizeof(request)) == -1) {
        perror("Sending request");
        exit(1);
    }
    close(serverFifoFD);
}

tlv_reply_t awaitReply(char* userFifoPath) {
    int userFifoFD = open(userFifoPath, O_RDONLY | O_NONBLOCK);
    if (userFifoFD == -1) {
        perror("Opening User Fifo");
        exit(1);
    }

    tlv_reply_t reply;

    if (read(userFifoFD, &reply.type, sizeof(op_type_t)) == -1) {
        perror("Reading reply type");
        exit(1);
    }

    if(read(userFifoFD, &reply.length, sizeof(uint32_t)) == -1) {
        perror("Reading reply length");
        exit(1);
    }
    
    if (read(userFifoFD, &reply.value, reply.length) == -1) {
        perror("Reading reply value");
        exit(1);
    }
    alarm(0);
    close(userFifoFD);
    return reply;
}

tlv_request_t createRequest(UserArgs args) {
    if (args.opcode < 0 || args.opcode >= __OP_MAX_NUMBER) {
        printf("Unknown optype: %d\n", args.opcode);
    }
    req_header_t header = makeReqHeader(args);
    
    req_value_t value;
    value.header = header;

    if (args.opcode == OP_CREATE_ACCOUNT)
        value.create = makeCreateAccReq(args.args);
    else if (args.opcode == OP_TRANSFER)
        value.transfer = makeTransferReq(args.args);

    tlv_request_t request;
    request.type = args.opcode;
    request.length = sizeof(value);
    request.value = value;
    return request;
}

req_header_t makeReqHeader(UserArgs args) {
    req_header_t header;
    header.pid = getpid();
    header.account_id = args.id;
    header.op_delay_ms = args.delayMS;
    strcpy(header.password, args.password);
    return header;
}

req_create_account_t makeCreateAccReq(char* args) {
    char* accId = strtok(args, " ");
    char* balance = strtok(NULL, " ");
    char* password = strtok(NULL, " ");
    if (accId == NULL || balance == NULL || password == NULL) {
        printf("Invalid create argument format: %s\n", args);
        exit(1);
    }
    req_create_account_t createAcc;
    createAcc.account_id = atoi(accId);
    createAcc.balance = atoi(balance);
    strcpy(createAcc.password, password);
    if (createAcc.account_id <= 0 || createAcc.account_id > MAX_BANK_ACCOUNTS) {
        printf("Invalid account ID. Args: %s\n", args);
        exit(1);
    }
    if (createAcc.balance <= 0 || createAcc.balance > MAX_BALANCE) {
        printf("Invalid balance. Args: %s\n", args);
        exit(1);
    }
    if (strlen(createAcc.password) < MIN_PASSWORD_LEN || strlen(createAcc.password) > MAX_PASSWORD_LEN) {
        printf("Invalid password: %s. Note that password size must have between %d and %d characters, and no spaces.\n", password, MIN_PASSWORD_LEN, MAX_PASSWORD_LEN);
        exit(1);
    }
    return createAcc;
}

req_transfer_t makeTransferReq(char* args) {
    char* accId = strtok(args, " ");
    char* amount = strtok(NULL, " ");
    if (amount == NULL || accId == NULL) {
        printf("Invalid transfer request argument format: %s\n", args);
        exit(1);
    }
    req_transfer_t transfer;
    transfer.account_id = atoi(accId);
    transfer.amount = atoi(amount);
    if (transfer.account_id <= 0 || transfer.account_id > MAX_BANK_ACCOUNTS) {
        printf("Invalid account ID. Args: %s\n", args);
        exit(1);
    }
    if (transfer.amount <= 0 || transfer.amount > MAX_BALANCE) {
        printf("Invalid amount. Args: %s\n", args);
        exit(1);
    }
    return transfer;
}