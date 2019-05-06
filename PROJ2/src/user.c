#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "sope.h"
#include "constants.h"


typedef struct {
    int id;
    char* password;
    int delayMS;
    int opcode;
    char* args;
} UserArgs;

tlv_request_t createRequest(UserArgs args);
req_header_t makeReqHeader(UserArgs args);
req_create_account_t makeCreateAccReq(char* args);
req_transfer_t makeTransferReq(char* args);
UserArgs processArgs(char* argv[]);

bool hasOuterQuotes(char* string);
char* removeOuterQuotes(char* string);



int main(int argc, char * argv[]) {

    if (argc != 6) {
        printf("Usage: ./user <ID> \"<Password>\" <Delay> <OPcode> \"<Args>\"");
        exit(1);
    }

    UserArgs args = processArgs(argv);
    tlv_request_t request = createRequest(args);


    printf("Pid = %d | AccID = %d | Delay = %d | Pass = \"%s\"\n", request.value.header.pid, request.value.header.account_id, request.value.header.op_delay_ms, request.value.header.password);
    printf("Type = %d | Length = %d \n", request.type, request.length);
    if (args.opcode == OP_CREATE_ACCOUNT)
        printf("Create ACC: ID = %d | Bal = %d | Pass = \"%s\"\n", request.value.create.account_id, request.value.create.balance, request.value.create.password);
    if (args.opcode == OP_TRANSFER)
        printf("Transfer: ID = %d | amount = %d \n",  request.value.transfer.account_id, request.value.transfer.amount);

}

UserArgs processArgs(char* argv[]) {
    UserArgs args;
    args.id = atoi(argv[1]);
    args.password = argv[2];
    args.delayMS = atoi(argv[3]);
    args.opcode = atoi(argv[4]);
    args.args = argv[5];
    return args;
}

tlv_request_t createRequest(UserArgs args) {
    if (args.opcode >= __OP_MAX_NUMBER) {
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
    if (strlen(args) < WIDTH_ACCOUNT + WIDTH_BALANCE + 2) {
        printf("Error making create account request: %s\n", args);
        exit(1);
    }
    char accId[WIDTH_ACCOUNT + 1] = "\0";
    char balance[WIDTH_BALANCE + 1] = "\0";
    char password[MAX_PASSWORD_LEN + 1] = "\0";
    strncpy(accId, args, WIDTH_ACCOUNT);
    args = args + WIDTH_ACCOUNT + 1;
    strncpy(balance, args, WIDTH_BALANCE);
    args = args + WIDTH_BALANCE + 1;
    strncpy(password, args, strlen(args));

    req_create_account_t createAcc;
    createAcc.account_id = atoi(accId);
    createAcc.balance = atoi(balance);
    strcpy(createAcc.password, password);
    return createAcc;
}

req_transfer_t makeTransferReq(char* args) {
    if (strlen(args) != WIDTH_ACCOUNT + WIDTH_BALANCE + 1) {
        printf("Error making create transfer request: %s\n", args);
        exit(1);
    }
    char accId[WIDTH_ACCOUNT + 1] = "\0";
    char amount[WIDTH_BALANCE + 1] = "\0";
    strncpy(accId, args, WIDTH_ACCOUNT);
    args = args + WIDTH_ACCOUNT + 1;
    strncpy(amount, args, WIDTH_BALANCE);

    req_transfer_t transfer;
    transfer.account_id = atoi(accId);
    transfer.amount = atoi(amount);
    return transfer;
}