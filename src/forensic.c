#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/wait.h>


// Parte 1

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
} forensicArgs;

void processArgs(forensicArgs * args, int argc, char* argv[], char* envp[]);

// Parte 2

int scanfile(char* filename, forensicArgs * args);
char* getFileType(char* filename);
char* getFileAccess(struct stat st);
char* getCheckSum(char* filename, int type);
char* timestampToISO(time_t timestamp);

// Parte 3

int scanDir(char* dirname, forensicArgs * args);

int main(int argc, char* argv[], char* envp[]) {
    forensicArgs args;
    processArgs(&args, argc, argv, envp);
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
}

// Parte 2 - Extrair  a  informação  solicitada  de  apenas  um  ficheiro  e  imprimi-la  na  saída  padrão de  acordo  com  os argumentos passados.
//         - Efetuar  o  mesmo  procedimento  mas  agora,  implementando  a  operação  da  opção  '-o'  (escrita  no  ficheiro designado).
int scanfile(char* filename, forensicArgs * args) {
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
    sprintf(fileInfo, "%s,%s,%ld,%s,%s,%s", filename, file_type, st.st_size, file_access, changeTime, modifyTime);
    //write(STDOUT_FILENO, fileInfo, strlen(fileInfo));
    free(file_type);
    free(file_access);
    free(changeTime);
    free(modifyTime);
    // Gets and prints the checksums if requested
    if (args->h) {
        char* checkSum;
        if (args->md5) {
            checkSum = getCheckSum(filename, 0);
            strcat(fileInfo, ",");
            strcat(fileInfo, checkSum);
           //sprintf(fileInfo, "%s,%s", fileInfo, checkSum);
            //write(STDOUT_FILENO, ",", 1);
            //write(STDOUT_FILENO, checkSum, strlen(checkSum));
            free(checkSum);
        }

        if (args->sha1) {
            checkSum = getCheckSum(filename, 1);
            strcat(fileInfo, ",");
            strcat(fileInfo, checkSum);
            //write(STDOUT_FILENO, ",", 1);
            //write(STDOUT_FILENO, checkSum, strlen(checkSum));
            free(checkSum);
        }

        if (args->sha256) {
            checkSum = getCheckSum(filename, 2);
            strcat(fileInfo, ",");
            strcat(fileInfo, checkSum);
            //write(STDOUT_FILENO, ",", 1);
            //write(STDOUT_FILENO, checkSum, strlen(checkSum));
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
    return 0;  
}

char* getFileType(char* filename) {
    char* str = malloc(100);
    FILE* fp;
    char command[100];
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
    char* str = malloc(100);
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

char* getCheckSum(char* filename, int type) {
    char* str = malloc(200);
    FILE* fp;
    char command[100];    
    // Creates and calls md5sum/sha1sum/sha256sum command to get sum
    if (type == 0)
        sprintf(command, "md5sum %s", filename);
    else if (type == 1)
        sprintf(command, "sha1sum %s", filename);
    else if (type == 2)
        sprintf(command, "sha256sum %s", filename);
    else {
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
    char* str = malloc(100);
    struct tm* t = localtime(&timestamp);
    sprintf(str, "%d-%02d-%02dT%02d:%02d:%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    return str;
}

// Parte 3 - Repetir o passo anterior para todos os ficheiros de um diretório.

int scanDir(char * dirname, forensicArgs * args) {
    DIR *dir = opendir(dirname);
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        if (ent->d_type == DT_REG) {
            char * newDir = (char *) malloc(100);
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
                char * newDir = (char *) malloc(strlen(dirname) + strlen(ent->d_name) + 5);
                sprintf(newDir, "%s/%s", dirname, ent->d_name);
                return scanDir(newDir, args);
            }
        }
    }
    closedir(dir);
    wait(NULL);
    return 0;
}
