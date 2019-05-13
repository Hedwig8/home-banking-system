#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dataManip.h"

// current number of threads
int threadNum;

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

    if(strlen(argv[2]) < MIN_PASSWORD_LEN || strlen(argv[2]) > MAX_PASSWORD_LEN) {
        write(STDERR_FILENO, "password length must be between 8 and 20 characters\n", 53);
        return 1;
    }
    createAdminAccount(argv[2]);
}

void* thr_fifo_answer(void * arg) {
    tlv_request_t req = *(tlv_request_t *) arg;
}

int main(int argc, char ** argv) {

    srand(time(NULL));

    if (argumentHandler(argc, argv)) // handles the arguments and creates admin account
        exit(1);

    // creates and opens FIFO to communication

    if(open(SERVER_FIFO_PATH, O_RDONLY | O_CREAT | O_EXCL, 0666) == -1) 
        exit(1);

    sleep(10);

    unlink(SERVER_FIFO_PATH);

    return 0;
}