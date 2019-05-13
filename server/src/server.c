#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "dataManip.h"
#include "queue.h"

// current number of threads
int threadNum;
Queue q;
bool srvShutdown = false;

int argumentHandler(int argc, char **argv)
{
    if (argc != 3)
    {
        write(STDERR_FILENO, "usage: server <no of bank offices> <admin password>\n", 53);
        return 1;
    }

    threadNum = atoi(argv[1]);
    if (threadNum <= 0 || threadNum > MAX_BANK_OFFICES)
    {
        write(STDERR_FILENO, "range of bank offices must be between 1 and 99, inclusive\n", 59);
        return 1;
    }

    if (strlen(argv[2]) < MIN_PASSWORD_LEN || strlen(argv[2]) > MAX_PASSWORD_LEN)
    {
        write(STDERR_FILENO, "password length must be between 8 and 20 characters\n", 53);
        return 1;
    }
    createAdminAccount(argv[2]);
}

void *thr_fifo_answer(void *arg)
{
    int num = *(int *)arg;
    fprintf(stdout, "thread: %d\n", num);
}

int main(int argc, char **argv)
{

    srand(time(NULL));

    if (argumentHandler(argc, argv)) // handles the arguments and creates admin account
        exit(1);

    // creates and opens FIFO to communication
    int fifoFd;
    if ((fifoFd = open(SERVER_FIFO_PATH, O_RDONLY | O_CREAT | O_EXCL, 0666)) == -1)
        exit(1);

    // creation of threads
    pthread_t threads[threadNum];
    for (int i = 0; i < threadNum; i++)
    {
        pthread_create(&threads[i], NULL, thr_fifo_answer, &i);
        fprintf(stdout, "loop: %d\n", i);
    }

    // initialization of queue
    queueInit(&q, sizeof(tlv_request_t));

    // waits for request on fifo and when reads, put in queue
    while (!srvShutdown)
    {
        // reads request
        tlv_request_t *req = NULL;
        read(fifoFd, req, sizeof(req));

        // if request 
        if(&req != NULL) {
            enqueue(&q, req);
        }
    }

    for (int i = 0; i < threadNum; i++)
    {
        pthread_join(threads[i], NULL);
    }

    unlink(SERVER_FIFO_PATH);

    return 0;
}