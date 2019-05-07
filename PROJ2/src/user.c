#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#include "logfileopeners.h"
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

static tlv_request_t request;
static tlv_reply_t reply;

int main(int argc, char * argv[]) {
    UserArgs args = processUserArgs(argc, argv);
    openULog();

    request = createRequest(args);
    
    char* userFifoPath = makeUserFifo();
    char* serverFifoPath = SERVER_FIFO_PATH;

    sendRequest(request, serverFifoPath);

    printf("Pid = %d | AccID = %d | Delay = %d | Pass = \"%s\"\n", request.value.header.pid, request.value.header.account_id, request.value.header.op_delay_ms, request.value.header.password);
    printf("Type = %d | Length = %d \n", request.type, request.length);
    if (args.opcode == OP_CREATE_ACCOUNT)
        printf("Create ACC: ID = %d | Bal = %d | Pass = \"%s\"\n", request.value.create.account_id, request.value.create.balance, request.value.create.password);
    if (args.opcode == OP_TRANSFER)
        printf("Transfer: ID = %d | amount = %d \n",  request.value.transfer.account_id, request.value.transfer.amount);
   
    tlv_reply_t reply = awaitReply(userFifoPath);

    deleteUserFifo();

    // Debug :D


}

void sigAlarmHandler() {
    printf("FIFO TIMEOUT. Exiting...\n");
    logEmptyReply(getULogFD(), getpid(), SRV_TIMEOUT, request);
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
    logRequest(getULogFD(), getpid(), &request);
    int serverFifoFD = open(serverFifoPath, O_WRONLY | O_NONBLOCK);
    if (serverFifoFD == -1) {
        perror("Error opening Server Fifo");
        logEmptyReply(getULogFD(), getpid(), SRV_DOWN, request);
        deleteUserFifo();
        exit(0);
    }
    /*
    if (write(serverFifoFD, &request, sizeof(op_type_t) + sizeof(uint32_t) + sizeof(req_header_t)) == -1) {
        perror("Error sending request header");
        exit(1);
    }
    if (request.type == OP_CREATE_ACCOUNT && write(serverFifoFD, &request.value.create, sizeof(req_create_account_t)) == -1) {
        perror("Error sending create acc");
        exit(1);
    }
    if (request.type == OP_TRANSFER && write(serverFifoFD, &request.value.transfer, sizeof(req_transfer_t)) == -1) {
        perror("Error sending transfer");
        exit(1);
    }
    */

    if (write(serverFifoFD, &request, sizeof(op_type_t) + sizeof(uint32_t) + request.length) == -1) {
        perror("Error sending request header");
        exit(1);
    }

    close(serverFifoFD);
}

tlv_reply_t awaitReply(char* userFifoPath) {
    enableTimeoutAlarm();    
    int userFifoFD = open(userFifoPath, O_RDONLY);
    if (userFifoFD == -1) {
        perror("Opening User Fifo");
        exit(1);
    }

    tlv_reply_t reply;
    
    if (read(userFifoFD, &reply, sizeof(op_type_t) + sizeof(uint32_t)) == -1) {
        perror("Error reading reply type length");
        exit(1);
    }

    if(read(userFifoFD, &reply.value, reply.length) == -1) {
        perror("Error reading reply balance");
        exit(1);
    }
    
    /*
    if (read(userFifoFD, &reply, sizeof(op_type_t) + sizeof(uint32_t) + sizeof(rep_header_t)) == -1) {
        perror("Error reading reply header");
        exit(1);
    }

    if(request.type == OP_BALANCE && read(userFifoFD, &reply.value.balance, sizeof(rep_balance_t)) == -1) {
        perror("Error reading reply balance");
        exit(1);
    }
    
    if(request.type == OP_TRANSFER && read(userFifoFD, &reply.value.transfer, sizeof(rep_transfer_t)) == -1) {
        perror("Error reading reply transfer");
        exit(1);
    }
    
    if(request.type == OP_SHUTDOWN && read(userFifoFD, &reply.value.shutdown, sizeof(rep_shutdown_t)) == -1) {
        perror("Error reading reply shutdown");
        exit(1);
    }
    */
    
    alarm(0);
    close(userFifoFD);
    logReply(getULogFD(), getpid(), &reply);
    return reply;
}

tlv_request_t createRequest(UserArgs args) {
    if (args.opcode < 0 || args.opcode >= __OP_MAX_NUMBER) {
        printf("Unknown optype: %d\n", args.opcode);
    }
    req_header_t header = makeReqHeader(args);    
    req_value_t value;
    tlv_request_t request;

    value.header = header;

    if (args.opcode == OP_CREATE_ACCOUNT) {
        value.create = makeCreateAccReq(args.args);
        request.length = sizeof(req_header_t) + sizeof(req_create_account_t);
    }
    else if (args.opcode == OP_TRANSFER) {
        value.transfer = makeTransferReq(args.args);
        request.length = sizeof(req_header_t) + sizeof(req_transfer_t);
    }
    else request.length = sizeof(req_header_t);

    request.type = args.opcode;
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