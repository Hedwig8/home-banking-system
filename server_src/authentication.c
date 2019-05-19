#include "authentication.h"

const char alphaNum[] = "0123456789abcdef";

char* getSaltKey() {
    int length = strlen(alphaNum);
    char *salt = calloc(SALT_LEN, sizeof(char));

    for(int i = 0; i < SALT_LEN; i++) {
        salt[i] = alphaNum[rand() % length];
    }

    return salt;
}

int calculateHash(char* iPassword, char* salt, char* oHash) {
    char hash[HASH_LEN+1];
    char toBeHashed[strlen(iPassword) + SALT_LEN+ 1];

    strcpy(toBeHashed, iPassword);
    strcat(toBeHashed, salt);

    //pipe to catch child's output
    int pipe1[2]; // pipe between child processes
    int pipe2[2]; // pipe between sha256sum and parent (this)

    //
    // ECHO -N $(iPassword)
    //

    if (pipe(pipe1) == -1) {
        perror("Pipe1");
        return 1;
    }

    pid_t pid1 = fork();

    //if error
    if (pid1 < 0)
    {
        perror("Fork error");
        return 1;
    }
    //if child
    else if (pid1 == 0)
    {
        // redirect output
        dup2(pipe1[WRITE], STDOUT_FILENO);
        //closing pipes
        close(pipe1[WRITE]);
        close(pipe1[READ]);
        // execute sha256sum command 
        execlp("echo", "echo", "-n", toBeHashed, NULL);
        exit(1);
    }

    //parent here

    //
    // SHA256SUM $(iPassword)
    //

    if(pipe(pipe2) == -1) {
        perror("Pipe2");
        return 1;
    }

    pid_t pid2 = fork();

    //if parent
    if (pid2 > 0)
    {
        //close pipes
        close(pipe1[WRITE]);
        close(pipe1[READ]);
        close(pipe2[WRITE]);
        // catch child's output
        read(pipe2[READ], hash, sizeof hash);
        close(pipe2[READ]);
    }
    //if child
    else if (pid2 == 0)
    {
        // redirect file descritors
        dup2(pipe1[READ], STDIN_FILENO);
        dup2(pipe2[WRITE], STDOUT_FILENO);
        // close pipe
        close(pipe1[WRITE]);
        close(pipe1[READ]);
        close(pipe2[WRITE]);
        close(pipe2[READ]);
        // execute sha256sum command 
        execlp("sha256sum", "sha256sum", NULL);
        exit(1);
    }
    else
    {
        perror("Fork error");
        return 1;
    }

    // make sure the hash has not too many chars
    strcpy(oHash, strtok(hash, " "));

    return 0;
}

int checkLogin(char* password, bank_account_t acc) {

    char hash[HASH_LEN];
    calculateHash(password, acc.salt, hash);

    if(strcmp(hash, acc.hash) != 0) 
        return 1;

    return 0;
}