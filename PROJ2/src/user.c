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
#include "replymaker.h"

tlv_request_t createRequest(UserArgs args);
req_header_t makeReqHeader(UserArgs args);
req_create_account_t makeCreateAccReq(char* args);
req_transfer_t makeTransferReq(char* args);

void sendRequest(tlv_request_t request, char* serverFifoPath);
tlv_reply_t awaitReply(char* userFifoPath);

void sigAlarmHandler();
void enableTimeoutAlarm();

void displayRequest(tlv_request_t request);
void displayReply(tlv_reply_t reply);

static tlv_request_t request;
static tlv_reply_t reply;

int main(int argc, char * argv[]) {
    UserArgs args = processUserArgs(argc, argv);
    openULog();

    request = createRequest(args);

    displayRequest(request);
    
    char* userFifoPath = makeUserFifo();
    char* serverFifoPath = SERVER_FIFO_PATH;

    sendRequest(request, serverFifoPath);
   
    reply = awaitReply(userFifoPath);
    
    deleteUserFifo();

    displayReply(reply);
}

void displayRequest(tlv_request_t request) {
    printf("\nSent request!\n");
    printf(" - PID: %d | Delay: %d | Message length: %d\n",  request.value.header.pid, request.value.header.op_delay_ms, request.length);
    printf(" - Account ID: %d | Password: \"%s\"\n", request.value.header.account_id, request.value.header.password);
    switch(request.type) {
        case OP_CREATE_ACCOUNT:
            printf("Type: ACCOUNT CREATION\n");
            printf(" - ID: %d | Password: \"%s\" | Initial Balance: %d€\n", request.value.create.account_id, request.value.create.password, request.value.create.balance);
            break;
        case OP_BALANCE: 
            printf("Type: BALANCE CHECK\n");
            break;
        case OP_TRANSFER:
            printf("Type: ACCOUNT TRANSFER\n");
            printf(" - ID: %d | Amount: %d€\n", request.value.transfer.account_id, request.value.transfer.amount);
            break;
        case OP_SHUTDOWN:
            printf("Type: SERVER SHUTDOWN\n");
            break;
        case __OP_MAX_NUMBER: break;
    }
    printf("\n------------------\n");
}

void displayReply(tlv_reply_t reply) {
    printf("\nReceived reply!\n");
    printf(" - Message length: %d\n", reply.length);
    printf(" - Account ID: %d\n", request.value.header.account_id);
    switch(reply.value.header.ret_code) {
        case RC_OK:
            printf("Status: OK\n");
            break;
        case RC_SRV_DOWN:
            printf("Status: Server Down\n");
            return;
        case RC_SRV_TIMEOUT:
            printf("Status: Server Timeout - Did not get response from server\n");
            return;
        case RC_USR_DOWN:
            printf("Status: User Down - Could not connect with user.\n");
            return;
        case RC_LOGIN_FAIL:
            printf("Status: Login Fail - Account ID and password don't match.\n");
            return;
        case RC_OP_NALLOW:
            printf("Status: Operation not allowed.\n");
            return;
        case RC_ID_IN_USE:
            printf("Status: Account ID in use.\n");
            return;
        case RC_ID_NOT_FOUND:
            printf("Status: Account ID could not be found.\n");
            return;           
        case RC_SAME_ID:
            printf("Status: Sender and Receiver have the same ID.\n");
            return;           
        case RC_NO_FUNDS:
            printf("Status: Sender has not enough funds.\n");
            return;           
        case RC_TOO_HIGH:
            printf("Status: Receiver would have too many funds.\n");
            return;           
        case RC_OTHER:
            printf("Status: Unexpected error.\n");
            return;     
    }
    switch(reply.type) {
        case OP_CREATE_ACCOUNT:
            printf("Type: ACCOUNT CREATION\n");
            break;
        case OP_BALANCE: 
            printf("Type: BALANCE CHECK\n");
            printf(" - Balance: %d€\n", reply.value.balance.balance);
            break;
        case OP_TRANSFER:
            printf("Type: ACCOUNT TRANSFER\n");
            printf(" - Balance: %d€\n", reply.value.transfer.balance);
            break;
        case OP_SHUTDOWN:
            printf("Type: SERVER SHUTDOWN");
            printf(" - Number of active offices: %d\n", reply.value.shutdown.active_offices);
            break;
        case __OP_MAX_NUMBER: break;
    }
}


void sigAlarmHandler() {
    reply = makeErrorReply(SRV_TIMEOUT, request);
    logReply(getULogFD(), getpid(), &reply);
    deleteUserFifo();
    displayReply(reply);
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

void disableTimeoutAlarm() {
    alarm(0);
}

void sendRequest(tlv_request_t request, char* serverFifoPath) {
    logRequest(getULogFD(), getpid(), &request);
    int serverFifoFD = open(serverFifoPath, O_WRONLY | O_NONBLOCK);
    if (serverFifoFD == -1) {
        reply = makeErrorReply(SRV_DOWN, request);
        displayReply(reply);
        logReply(getULogFD(), getpid(), &reply);
        deleteUserFifo();
        exit(0);
    }

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
        return makeErrorReply(USR_DOWN, request);
    }

    tlv_reply_t reply = newReply();
    
    if (read(userFifoFD, &reply, sizeof(op_type_t) + sizeof(uint32_t)) == -1) {
        perror("Error reading reply type length");
        exit(1);
    }

    if(read(userFifoFD, &reply.value, reply.length) == -1) {
        perror("Error reading reply balance");
        exit(1);
    }
        
    disableTimeoutAlarm();
    close(userFifoFD);
    logReply(getULogFD(), getpid(), &reply);
    return reply;
}

tlv_request_t createRequest(UserArgs args) {
    if (args.opcode < 0 || args.opcode >= __OP_MAX_NUMBER) {
        printf("Unknown optype: %d\n", args.opcode);
        exit(1);
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
    if (header.account_id < 0 || header.account_id > MAX_BANK_ACCOUNTS) {
        printf("Invalid account ID: %d\n", header.account_id);
        exit(1);
    }
    if (header.op_delay_ms < 0) {
        printf("Invalid delay: %d\n", header.op_delay_ms);
        exit(1);
    }
    strcpy(header.password, args.password);
    return header;
}

req_create_account_t makeCreateAccReq(char* args) {
    char* accId = strtok(args, " ");
    char* balance = strtok(NULL, " ");
    char* password = strtok(NULL, " ");
    if (accId == NULL || balance == NULL || password == NULL) {
        printf("Invalid arguments: %s\n", args);
        printf("Create Account Usage: ./user <ID> \"<Password>\" <Delay> 0 \"<ID> <Balance> <Password>\"\n");
        exit(1);
    }
    req_create_account_t createAcc;
    createAcc.account_id = atoi(accId);
    createAcc.balance = atoi(balance);
    strcpy(createAcc.password, password);
    if (createAcc.account_id <= 0 || createAcc.account_id > MAX_BANK_ACCOUNTS) {
        printf("Invalid account ID: %s\n", accId);
        printf("Note that ID must be between %d and %d.\n", 1, MAX_BANK_ACCOUNTS);
        printf("Create Account Usage: ./user <ID> \"<Password>\" <Delay> 0 \"<ID> <Balance> <Password>\"\n");
        exit(1);
    }
    if (createAcc.balance <= 0 || createAcc.balance > MAX_BALANCE) {
        printf("Invalid balance: %s\n", balance);
        printf("Note that balance must be between %d and %ld.\n", 1, MAX_BALANCE);
        printf("Create Account Usage: ./user <ID> \"<Password>\" <Delay> 0 \"<ID> <Balance> <Password>\"\n");
        exit(1);
    }
    if (strlen(createAcc.password) < MIN_PASSWORD_LEN || strlen(createAcc.password) > MAX_PASSWORD_LEN) {
        printf("Invalid password: %s.\n", password);
        printf("Note that password size must have between %d and %d characters, without spaces.\n", MIN_PASSWORD_LEN, MAX_PASSWORD_LEN);
        printf("Create Account Usage: ./user <ID> \"<Password>\" <Delay> 0 \"<ID> <Balance> <Password>\"\n");
        exit(1);
    }
    return createAcc;
}

req_transfer_t makeTransferReq(char* args) {
    char* accId = strtok(args, " ");
    char* amount = strtok(NULL, " ");
    if (amount == NULL || accId == NULL) {
        printf("Invalid arguments: %s\n", args);
        printf("Transfer Usage: ./user <ID> \"<Password>\" <Delay> 2 \"<ID> <Amount>\"\n");
        exit(1);
    }
    req_transfer_t transfer;
    transfer.account_id = atoi(accId);
    transfer.amount = atoi(amount);
    if (transfer.account_id <= 0 || transfer.account_id > MAX_BANK_ACCOUNTS) {
        printf("Invalid account ID: %s\n", accId);
        printf("Transfer Usage: ./user <ID> \"<Password>\" <Delay> 2 \"<ID> <Amount>\"\n");
        exit(1);
    }
    if (transfer.amount <= 0 || transfer.amount > MAX_BALANCE) {
        printf("Invalid amount: %s\n", amount);
        printf("Transfer Usage: ./user <ID> \"<Password>\" <Delay> 2 \"<ID> <Amount>\"\n");
        exit(1);
    }
    return transfer;
}