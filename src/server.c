
// provided files
#include <sope.h>

#define WRITE 1
#define READ 0


// array of accounts
//void *accounts[MAX_BANK_ACCOUNTS] = {NULL};
bank_account_t accounts[MAX_BANK_ACCOUNTS] = {NULL};
int threadNum;

int calculateHash(char* iPassword, char* oHash) {
    char hash[HASH_LEN+1];

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
        execlp("echo", "echo", "-n", iPassword, NULL);
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

    //strcpy(outputStr, strtok(outputStr, "\n"));

    return 0;
}

void createAdminAccount(char password[]) {

    bank_account_t adminAcc;
    adminAcc.account_id = ADMIN_ACCOUNT_ID;
    adminAcc.balance = 0;


}

int argumentHandler(int argc, char ** argv) {
    if(argc != 3) return 1;

    threadNum = atoi(argv[1]);
    if(threadNum <= 0 || threadNum > MAX_BANK_OFFICES) return 1;

    createAdminAccount(argv[2]);
}

int createAccount(int accountID, char* password, int value) {

}

int main(int argc, char ** argv) {

    if (argumentHandler(argc, argv))
        exit(1);

    return 0;
}