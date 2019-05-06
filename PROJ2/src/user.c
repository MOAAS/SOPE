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

char* makeUserFifo();

int main(int argc, char * argv[]) {

    if (argc != 6) {
        printf("Usage: ./user <ID> \"<Password>\" <Delay> <OPcode> \"<Args>\"");
        exit(1);
    }

    UserArgs args = processArgs(argv);
    tlv_request_t request = createRequest(args);

    // Falta verificar se pedido e valido... :D
    char* userFifoPath = makeUserFifo();
    char* serverFifoPath = SERVER_FIFO_PATH;

    // Enviar...

   // int serverFifoFD = open(serverFifoPath, O_WRONLY);

   // RECEBER!
    //int userFifoFD = open(userFifoPath, O_RDONLY);

    printf("Pid = %d | AccID = %d | Delay = %d | Pass = \"%s\"\n", request.value.header.pid, request.value.header.account_id, request.value.header.op_delay_ms, request.value.header.password);
    printf("Type = %d | Length = %d \n", request.type, request.length);
    if (args.opcode == OP_CREATE_ACCOUNT)
        printf("Create ACC: ID = %d | Bal = %d | Pass = \"%s\"\n", request.value.create.account_id, request.value.create.balance, request.value.create.password);
    if (args.opcode == OP_TRANSFER)
        printf("Transfer: ID = %d | amount = %d \n",  request.value.transfer.account_id, request.value.transfer.amount);

}

char* makeUserFifo() {
    char* userFifoPath = malloc(USER_FIFO_PATH_LEN + 1);
    char userPID[WIDTH_ID + 1];
    sprintf(userPID, "%05d", getpid());
    strcpy(userFifoPath, USER_FIFO_PATH_PREFIX);
    strcat(userFifoPath, userPID);
    mkfifo(userFifoPath, 0777);
    printf("%s ", userFifoPath);
    return userFifoPath;
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