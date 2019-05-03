#include <stdio.h>

struct UserArgs
{
    int id;
    char* password;
    int delayMS;
    int opcode;
    char* args;
};

tlv_request_t createRequest(UserArgs args);
UserArgs processArgs(char* argv[]);

int main(int argc, char * argv[]) {

    if (argc != 6) {
        printf("Usage: ./user <ID> \"<Password>\" <Delay> <OPcode> \"<Args>\"");
        exit(1);
    }

    UserArgs args = processArgs(argv);
    tlv_request_t request = createRequest(args);

    printf("USER!\n");
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
    tlv_request_t request;
    request.type = args.opcode;

    req_header_t header;
    header.pid = getpid();
    header.account_id = args.id;
    header.op_delay_ms = args.delayMS;
    strcpy(header.password, args.password);
}