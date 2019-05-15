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
    int tid = *(int *)arg;

    //Isto deveria ser global para depois ser destruida no main?
    //A queue nessecita de mutex #420Questions
    //pthread_cond_t queueMutex = PTHREAD_COND_INITIALIZER;

    while(!srvShutdown) {
        while(!isEmptyQueue(&q)) {
            //pthread_mutex_lock (queueMutex);
            tlv_request_t req;
            dequeue(&q, &req);
            //fuc para tratar do pedido
            write(STDOUT_FILENO, "tlv received", 13);
            logRequest(STDOUT_FILENO, tid, &req);
            srvShutdown = true;
            //pthread_mutex_unlock (queueMutex);
        }
    }

    //pthread_mutex_destroy (queueMutex);
}

void processRequest (tlv_request_t req) {
    switch (req.type)
    {
    case OP_CREATE_ACCOUNT:
        break;

    case OP_BALANCE:
        break;

    case OP_TRANSFER:
        break;

    case OP_SHUTDOWN:
        if (req.value.header.account_id == 0)
            srvShutdown = true;
        break;

    default:
        break;
    }
}

void closeFd(int r, void* arg) {
    close(*(int*)arg);
}

void removePath(int r, void* arg) {
    unlink((char *) arg);
}

int main(int argc, char **argv)
{

    srand(time(NULL));

    if (argumentHandler(argc, argv)) // handles the arguments and creates admin account
        exit(1);

    // creates and opens FIFO to communication
    
    if(mkfifo(SERVER_FIFO_PATH, 0666) != 0)
    {
        write(STDERR_FILENO, "FIFO tmp/secure_srv could not be created successfully\n", 55);
        exit(1);
    } 
    else on_exit(removePath, SERVER_FIFO_PATH);
    int fifoFd= open(SERVER_FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fifoFd == -1)
        exit(1);

    // creation of threads
    pthread_t threads[threadNum];
    for (int i = 0; i < threadNum; i++)
    {
        int tid = i;
        pthread_create(&threads[i], NULL, thr_fifo_answer, &tid);
        fprintf(stdout, "loop: %d\n", i);
    }

    // initialization of queue
    queueInit(&q, sizeof(tlv_request_t));

    // waits for request on fifo and when reads, put in queue
    while (!srvShutdown)
    {
        // reads request
        tlv_request_t req;
        if ( read(fifoFd, &req, sizeof(tlv_request_t)) > 0)
        {
            enqueue(&q, &req);
        }
    }

    for (int i = 0; i < threadNum; i++)
    {
        pthread_join(threads[i], NULL);
    }

    unlink(SERVER_FIFO_PATH);

    return 0;
}