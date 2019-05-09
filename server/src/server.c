#include <stdlib.h>
#include <string.h>

// provided files
#include "sope.h"

#define WRITE 1
#define READ 0

// array of accounts
//void *accounts[MAX_BANK_ACCOUNTS] = {NULL};
bank_account_t accounts[MAX_BANK_ACCOUNTS] = {0};
int threadNum;
const char alphaNum[] = "0123456789abcdefghijklmnopqrstuwxyz";

int getSaltKey(char *oSalt) {
    int length = strlen(alphaNum);
    char salt[SALT_LEN];

    for(int i = 0; i < SALT_LEN; i++) {
        salt[i] = alphaNum[rand() % length];
    }

    write(STDOUT_FILENO, salt, SALT_LEN);
    write(STDOUT_FILENO, "\n", 1);

    strcpy(oSalt, salt);

    return 0;
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
    write(STDOUT_FILENO, hash, 64);
    write(STDOUT_FILENO, "\n", 1);

    return 0;
}

void createAdminAccount(char password[]) {

    bank_account_t adminAcc;
    adminAcc.account_id = ADMIN_ACCOUNT_ID;
    adminAcc.balance = 0;

    //char *hash, *salt;
    getSaltKey(adminAcc.salt);
    calculateHash(password, adminAcc.salt, adminAcc.hash);

    accounts[0] = adminAcc;

    write(STDOUT_FILENO, accounts[0].salt, 64);
    write(STDOUT_FILENO, "\n", 1);

    write(STDOUT_FILENO, accounts[0].hash, 64);
    write(STDOUT_FILENO, "\n", 1);
}

int argumentHandler(int argc, char ** argv) {
    if(argc != 3) {
        write(STDERR_FILENO, "usage: server <no of bank offices> <admin password>\n", 53);
        return 1;
    }

    threadNum = atoi(argv[1]);
    if(threadNum <= 0 || threadNum > MAX_BANK_OFFICES) {
        write(STDERR_FILENO, "range of bank offices must be between 1 and 99, inclusive\n", 59); 
        return 1;
    }

    createAdminAccount(argv[2]);
}

int createAccount(int accountID, char* password, int value) {

}

int main(int argc, char ** argv) {

    srand(time(NULL));

    if (argumentHandler(argc, argv)) // handles the arguments and creates admin account
        exit(1);

    return 0;
}