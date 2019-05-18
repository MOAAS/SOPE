#include "argProcessing.h"

ServerArgs processServerArgs(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: ./server <NUM_BANK_OFFICES> \"<ADMIN_PASSWORD>\"\n");
        exit(1);
    }

    ServerArgs args;
    args.numOffices = atoi(argv[1]);
    args.adminPassword = argv[2];

    if (args.numOffices <= 0 || args.numOffices > MAX_BANK_OFFICES) {
        printf("Invalid number of bank offices: %s\n", argv[1]);
        exit(1);
    }

    if (strlen(args.adminPassword) < MIN_PASSWORD_LEN || strlen(args.adminPassword) > MAX_PASSWORD_LEN) {
        printf("Invalid password length: %s\n", args.adminPassword);
        exit(1);
    }
    return args;
}

UserArgs processUserArgs(int argc, char* argv[]) {
    if (argc != 6) {
        printf("Usage: ./user <ID> \"<Password>\" <Delay> <OPcode> \"<Args>\"\n");
        exit(1);
    }
    UserArgs args;
    args.id = atoi(argv[1]);
    args.password = argv[2];
    args.delayMS = atoi(argv[3]);
    args.opcode = atoi(argv[4]);
    args.args = argv[5];
    return args;
}