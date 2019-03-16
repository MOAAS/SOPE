#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/times.h>

#define MAXCHAR 1000

typedef struct {
    bool r;
    bool h;
    bool md5, sha1, sha256;
    bool v;
    bool o;
    bool isDir;
    char* path;
    char* logFilePath;
    char* outFilePath;
    char* command;
} forensicArgs;

static int numDirs = 0;
static int numFiles = 0;
static clock_t startTime = 0;
static bool receivedSigInt = false;
static pid_t originalPID;
static forensicArgs args;

// Parte 1


void processArgs(forensicArgs * args, int argc, char* argv[], char* envp[]);

// Parte 2

typedef enum {
    md5,
    sha1,
    sha256
} fileSumType;

int scanfile(char* filename, forensicArgs * args);
char* getFileType(char* filename);
char* getFileAccess(struct stat st);
char* getCheckSum(char* filename, fileSumType type);
char* timestampToISO(time_t timestamp);

// Parte 3, 4, 5


int scanDir(char* dirname, forensicArgs * args);
void waitForChildren();

// Parte 6

typedef enum {
    command,
    sigReceive,
    fileScan
} logType;

void addLog(char* logFilePath, logType type, char* act);

// Parte 7

void sigintHandler(int signo);
void enableSigHandlers();

// Parte 8 

void sigUsr1Handler(int signo);
void sigUsr2Handler(int signo);
void signalUSR1();
void signalUSR2();

///////////////////////////


int main(int argc, char* argv[], char* envp[]) {
    startTime = times(NULL);
    processArgs(&args, argc, argv, envp);
    enableSigHandlers();
    originalPID = getpid();
    if (args.v)
        addLog(args.logFilePath, command, args.command);
    if (args.isDir)
        scanDir(args.path, &args);
    else scanfile(args.path, &args);
}

// Parte 1 - Receber, tratar e guardar os argumentos e variáveis de ambiente.
void processArgs(forensicArgs * args, int argc, char* argv[], char* envp[]) {
    args->r = false;
    args->h = false;
    args->md5 = false;
    args->sha1 = false;
    args->sha256 = false;
    args->o = false;
    args->logFilePath = NULL;
    args->outFilePath = NULL;
    args->path = NULL;    
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
            if(strstr(argv[i+1], "md5") != NULL)
                args->md5 = true;
            if(strstr(argv[i+1], "sha1") != NULL)
                args->sha1 = true;
            if(strstr(argv[i+1], "sha256") != NULL)
                args->sha256 = true;
            if (!args->md5 && !args->sha1 && !args->sha256) {
                printf("Error: Need to pick at least one hash (md5/sha1/sha256).\n");
                exit(2);
            }
            i++;
        }
        else if (strcmp(argv[i], "-o") == 0) {
            args->o = true;
            if (i == argc - 1) {
                printf("Error: Need to specify an output file.\n");
                exit(3);
            }
            args->outFilePath = argv[i + 1];
            FILE* outFile = fopen(args->outFilePath, "w");
            if (outFile == NULL) {
                perror(args->outFilePath);
                exit(4);
            }
            else {
                i++;       
                fclose(outFile);
            }
        }
        else if (strcmp(argv[i], "-v") == 0) {
            args->v = true;
            char* logfilepath = getenv("LOGFILENAME");
            if (logfilepath == NULL) {
                printf("Error: LOGFILENAME environment variable does not exist.\n");
                exit(5);
            }
            args->logFilePath = logfilepath;
            FILE* logFile = fopen(logfilepath, "w");
            if (logFile == NULL) {
                perror(logfilepath);
                exit(6);
            }
            else fclose(logFile);
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
    }
    else if (!S_ISREG(st.st_mode)) {
        printf("Error: %s is not directory or file!\n", pathname);
        exit(9);
    }
    args->path = pathname;
    // copy entire command to the struct too
    args->command = malloc(MAXCHAR);
    strcpy(args->command, argv[0]);
    for (int i = 1; i < argc; i++) {
        strcat(args->command, " ");
        strcat(args->command, argv[i]);
    }
}

// Parte 2 - Extrair  a  informação  solicitada  de  apenas  um  ficheiro  e  imprimi-la  na  saída  padrão de  acordo  com  os argumentos passados.
//         - Efetuar  o  mesmo  procedimento  mas  agora,  implementando  a  operação  da  opção  '-o'  (escrita  no  ficheiro designado).
int scanfile(char* filename, forensicArgs * args) {
    if (args->o)
        signalUSR2(); // Poe-se aqui??
    int saved_stdout = 0;
    // If -o is an option, redirect STDOUT to the specified file, saving STDOUT's file descriptor
    if (args->o) {
        saved_stdout = dup(STDOUT_FILENO);
        int fd_out = open(args->outFilePath, O_WRONLY | O_CREAT | O_APPEND);
        if (fd_out == -1) {
            perror(args->outFilePath);
            return 1;
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    // Opens the file
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror(filename);
        return 1;
    }
    struct stat st;
    // Gets and prints the desired info
    stat(filename, &st);
    char* file_type = getFileType(filename); 
    char* file_access = getFileAccess(st);
    char* changeTime = timestampToISO(st.st_ctime);
    char* modifyTime = timestampToISO(st.st_mtime);
    char fileInfo[1000];
    sprintf(fileInfo, "%s,%s,%ld,%s,%s,%s", filename, file_type, (long) st.st_size, file_access, changeTime, modifyTime);
    free(file_type);
    free(file_access);
    free(changeTime);
    free(modifyTime);
    // Gets and prints the checksums if requested
    if (args->h) {
        char* checkSum;
        if (args->md5) {
            checkSum = getCheckSum(filename, md5);
            strcat(fileInfo, ",");
            free(checkSum);
        }

        if (args->sha1) {
            checkSum = getCheckSum(filename, sha1);
            strcat(fileInfo, ",");
            strcat(fileInfo, checkSum);
            free(checkSum);
        }

        if (args->sha256) {
            checkSum = getCheckSum(filename, sha256);
            strcat(fileInfo, ",");
            strcat(fileInfo, checkSum);
            free(checkSum);
        }
    }
    strcat(fileInfo, "\n");
    write(STDOUT_FILENO, fileInfo, strlen(fileInfo));

    // If -o is an option, restore STDOUT
    if (args->o) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
    if (args->v)
        addLog(args->logFilePath, fileScan, filename);
    return 0;  
}

char* getFileType(char* filename) {
    char* str = malloc(MAXCHAR);
    FILE* fp;
    char command[MAXCHAR];
    // Calls file command to get file type
    sprintf(command, "file %s", filename);
    if ((fp = popen(command,"r")) == NULL) {
        printf("%s: command failed", command);
        return NULL;
    }
    // Reads the file type from pipe
    fread(str, sizeof(char), strlen(filename) + 2, fp);
    fgets(str, 1000, fp);

    int length = strlen(str);
    // Cleans up the string
    if (str[length - 1] == '\n')
        str[length - 1] = 0;
    for (int i = 0; i < length; i++) {
        if (str[i] == ',')
            str[i] = ' ';
    }
    // Closes the pipe
    fclose(fp);
    return str;
}

char* getFileAccess(struct stat st) {
    char* str = malloc(MAXCHAR);
    strcpy(str, "---------");
    if (st.st_mode & S_IRUSR)
        str[0] = 'r';
    if (st.st_mode & S_IWUSR)
        str[1] = 'w';    
    if (st.st_mode & S_IXUSR)
        str[2] = 'x';    
    if (st.st_mode & S_IRGRP)
        str[3] = 'r';    
    if (st.st_mode & S_IWGRP)
        str[4] = 'w';    
    if (st.st_mode & S_IXGRP)
        str[5] = 'x';    
    if (st.st_mode & S_IROTH)
        str[6] = 'r';    
    if (st.st_mode & S_IWOTH)
        str[7] = 'w';
    if (st.st_mode & S_IXOTH)
        str[8] = 'x';
    return str;
    
}

char* getCheckSum(char* filename, fileSumType type) {
    char* str = malloc(MAXCHAR);
    FILE* fp;
    char command[MAXCHAR];    
    // Creates and calls md5sum/sha1sum/sha256sum command to get sum
    switch (type) {
        case md5:
            sprintf(command, "md5sum %s", filename);
            break;
        case sha1:
            sprintf(command, "sha1sum %s", filename);
            break;
        case sha256:
            sprintf(command, "sha256sum %s", filename);
            break;
        default:
            printf("%d: Unknown sum type.\n", type);
            return NULL;
    }
    if ((fp = popen(command, "r")) == NULL) {
        printf("%s: command failed", command);
        return NULL;
    }
    // Reads the checksum from pipe
    fgets(str, 200, fp);
    int length = strlen(str);
    // Cleans up the string
    if (str[length - 1] == '\n')
        str[length - 1] = 0;
    str[length - strlen(filename) - 3] = '\0';
    // Closes the pipe
    fclose(fp);
    return str;
}

char* timestampToISO(time_t timestamp) {
    char* str = malloc(MAXCHAR);
    struct tm* t = localtime(&timestamp);
    sprintf(str, "%d-%02d-%02dT%02d:%02d:%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    return str;
}

// Parte 3/4/5 - Repetir o passo anterior para todos os ficheiros de um diretório, funcionalidade recursiva.

int scanDir(char * dirname, forensicArgs * args) {
    if (args->o)
        signalUSR1(); // Poe-se aqui??
    DIR *dir = opendir(dirname);
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && !receivedSigInt) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        if (ent->d_type == DT_REG) {
            char * newDir = (char *) malloc(MAXCHAR);
            sprintf(newDir, "%s/%s", dirname, ent->d_name);
            scanfile(newDir, args);
        }
        else if (ent->d_type == DT_DIR && args->r) {
            pid_t pid = fork();
            if (pid < 0) {
                printf("Error creating child process\n");
                exit(1);
            } else if (pid == 0) {
                continue;
            } else {
                char * newDir = (char *) malloc(MAXCHAR);
                sprintf(newDir, "%s/%s", dirname, ent->d_name);
                return scanDir(newDir, args);
            }
        }
    }
    closedir(dir);
    waitForChildren();
    return 0;
}

void waitForChildren() {
    pid_t wait_ret;
    do {
        wait_ret = wait(NULL);
    } while (errno != ECHILD && wait_ret != -1);
}

// Parte 6 - Adicionar as funcionalidades de registo (log).

void addLog(char* logFilePath, logType type, char* act) {
    FILE* logFile = fopen(logFilePath, "a");
    if (logFile == NULL)
        return;
    char logLine[MAXCHAR];
    clock_t now = times(NULL);
    switch (type) {
        case command:
            sprintf(logLine, "%f - %08d - %s %s\n",  (double)(now - startTime)/sysconf(_SC_CLK_TCK) * 1000, getpid(), "COMMAND", act);
            break;
        case sigReceive:
            sprintf(logLine, "%f - %08d - %s %s\n", (double)(now - startTime)/sysconf(_SC_CLK_TCK) * 1000, getpid(), "SIGNAL", act);
            break;
        case fileScan:
            sprintf(logLine, "%f - %08d - %s %s\n", (double)(now - startTime)/sysconf(_SC_CLK_TCK) * 1000, getpid(), "ANALYZED", act);
            break;

    }
    fwrite(logLine, sizeof(char), strlen(logLine), logFile);
    fclose(logFile);
}

// Parte 7 - Adicionar a funcionalidade de tratamento do sinal associado ao CTRL+C.

void sigintHandler(int signo) {
    receivedSigInt = true;
}

void enableSigHandlers() {
    struct sigaction sa;
    sa.sa_handler = sigintHandler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = sigUsr1Handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = sigUsr2Handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, NULL);    
}

// Parte 8 - Adicionar a funcionalidade de emissão e tratamento dos sinais SIGUSR1 e SIGUSR2.

void sigUsr1Handler(int signo) {
    numDirs++;
    printf("New directory: %d/%d directories/files at this time \n", numDirs, numFiles);
    if (args.v)
        addLog(args.logFilePath, sigReceive, "USR1");
}

void sigUsr2Handler(int signo) {
    numFiles++;
    printf("New file: %d/%d directories/files at this time \n", numDirs, numFiles);
    if (args.v)
        addLog(args.logFilePath, sigReceive, "USR2");
}

void signalUSR1() {
    kill(originalPID, SIGUSR1);    
}

void signalUSR2() {
    kill(originalPID, SIGUSR2);    
}