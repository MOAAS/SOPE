#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h> 
#include <sys/stat.h>

typedef struct {
    bool r;
    bool h;
    bool md5, sha1, sha256;
    bool v;
    bool o;
    bool isDir;
    DIR* dir;
    FILE* file;
    FILE* logFile;
    FILE* outFile;
} forensicArgs;

void processArgs(forensicArgs * args, int argc, char* argv[], char* envp[]);

void displayArgs(forensicArgs args) { // DEBOOG
    printf("r h md5 sha1 sha256 v o isDir\n");
    printf("%d %d %d   %d    %d      %d %d %d\n", args.r, args.h, args.md5, args.sha1, args.sha256, args.v, args.o, args.isDir);

}

int main(int argc, char* argv[], char* envp[]) {
    forensicArgs args;
    processArgs(&args, argc, argv, envp);
    displayArgs(args);
}


void processArgs(forensicArgs * args, int argc, char* argv[], char* envp[]) {
    args->r = false;
    args->h = false;
    args->md5 = false;
    args->sha1 = false;
    args->sha256 = false;
    args->o = false;
    args->logFile = NULL;
    args->outFile = NULL;
    args->file = NULL;    
    if (argc < 2) {
        printf("Usage: forensic [-r] [-h [md5[,sha1[,sha256]]] [-o <outfile>] [-v] <file|dir>\n");
        exit(1);
    }
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-r") == 0) {
            args->r = true;
        }
        else if (strcmp(argv[i], "-h") == 0) {
            args->h = true;
            while (i + 1 < argc) {
                if (strcmp(argv[i + 1], "md5") == 0)
                    args->md5 = true;
                else if (strcmp(argv[i + 1], "sha1") == 0)
                    args->sha1 = true;
                else if (strcmp(argv[i + 1], "sha256") == 0)
                    args->sha256 = true;
                else break;
                i++;
            }
            if (!args->md5 && !args->sha1 && !args->sha256) {
                printf("Error: Need to pick at least one hash (md5/sha1/sha256).\n");
                exit(2);
            }
        }
        else if (strcmp(argv[i], "-o") == 0) {
            args->o = true;
            if (i == argc - 1) {
                printf("Error: Need to specify an output file.\n");
                exit(3);
            }
            args->outFile = fopen(argv[i + 1], "w");
            if (args->outFile == NULL) {
                perror(argv[i+1]);
                exit(4);
            }
            else i++;       
        }
        else if (strcmp(argv[i], "-v") == 0) {
            args->v = true;
            char* logfilepath = getenv("LOGFILENAME");
            if (logfilepath == NULL) {
                printf("Error: LOGFILENAME environment variable does not exist.\n");
                exit(5);
            }
            args->logFile = fopen(logfilepath, "w");
            if (args->logFile == NULL) {
                perror(logfilepath);
                exit(6);
            }            
        }
        else {
            printf("Error: Unrecognized argument: %s \n", argv[i]);
            exit(7);
        }
    }
    // process last arg
    char* pathname = argv[argc - 1];
    struct stat st;
    if (stat(pathname, &st) == -1) {
        perror(pathname);
        exit(8);
    }
    if (S_ISDIR(st.st_mode)) {
        args->isDir = true;
        args->dir = opendir(pathname);
    }
    else if (S_ISREG(st.st_mode)) {
        args->file = fopen(pathname, "r");
    }
    else {
        printf("Error: %s is not directory or file!\n", pathname);
        exit(9);
    }

}