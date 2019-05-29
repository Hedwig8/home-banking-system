#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include "dataManip.h"
#include "queue.h"
#include "utils.h"

// current number of threads
int threadNum, logFd, fifoFd, emptyQueue;
Queue q;
bool srvShutdown = false;
pthread_mutex_t accountsAccessMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queueAccessMutex = PTHREAD_MUTEX_INITIALIZER; // CHANGE TO SEMAPHORE???
sem_t full, empty;

void processRequest(tlv_request_t *req, tlv_reply_t *rep, int tid)
{

    bool verification = true;
    bank_account_t acc;

    // check if exists
    if (findAccount(req->value.header.account_id, &acc))
    {
        rep->value.header.ret_code = RC_ID_NOT_FOUND;
        verification = false;
    }

    // check if pair (id, password) is correct
    if (checkLogin(req->value.header.password, acc))
    {
        rep->value.header.ret_code = RC_LOGIN_FAIL;
        verification = false;
    }

    rep->type = req->type;
    rep->value.header.account_id = req->value.header.account_id;

    // if error occured, prepare reply and return function
    if (!verification)
    {
        switch (req->type)
        {
        case OP_BALANCE:
            rep->value.balance.balance = 0;
            break;
        case OP_TRANSFER:
            rep->value.transfer.balance = 0;
            break;
        case OP_SHUTDOWN:
            rep->value.shutdown.active_offices = 0;
            break;
        default:
            break;
        }
        rep->length = sizeof(rep->value);
        return;
    }

    switch (req->type)
    {
    case OP_CREATE_ACCOUNT:
        // must be admin
        if (acc.account_id != 0)
        {
            rep->value.header.ret_code = RC_OP_NALLOW;
            break;
        }

        // delay
        logSyncDelay(logFd, tid, req->value.header.pid, req->value.header.op_delay_ms);
        usleep(req->value.header.op_delay_ms * 1000);
        // creates acc and fills rep's ret code
        createAccount(req->value.create, rep);

        break;

    case OP_BALANCE:
        // cannot be admin
        if (acc.account_id == 0)
        {
            rep->value.header.ret_code = RC_OP_NALLOW;
            break;
        }

        // delay
        logSyncDelay(logFd, tid, req->value.header.pid, req->value.header.op_delay_ms);
        usleep(req->value.header.op_delay_ms * 1000);

        getBalance(req->value.header, rep);

        break;

    case OP_TRANSFER:
        // cannot be admin
        if (acc.account_id == 0)
        {
            rep->value.header.ret_code = RC_OP_NALLOW;
            break;
        }

        // delay
        logSyncDelay(logFd, tid, req->value.header.pid, req->value.header.op_delay_ms);
        usleep(req->value.header.op_delay_ms * 1000);

        consumeTransfer(req->value.header, req->value.transfer, rep);

        break;

    case OP_SHUTDOWN:
        // must be admin
        if (acc.account_id != 0)
        {
            rep->value.header.ret_code = RC_OP_NALLOW;
            break;
        }

        // delay
        logDelay(logFd, tid, req->value.header.op_delay_ms);
        usleep(req->value.header.op_delay_ms * 1000);

        // changes fifo permissions to r--r--r--
        fchmod(fifoFd, 0444);

        // get active offices by calculating semaphores values
        int emptyValue, fullValue;
        sem_getvalue(&empty, &emptyValue);
        sem_getvalue(&full, &fullValue);
        rep->value.shutdown.active_offices = threadNum - emptyValue - fullValue;
        
        rep->value.header.ret_code = RC_OK;
        srvShutdown = true;
        break;

    default:
        break;
    }

    rep->length = sizeof(rep->value);
}

void sendReply(tlv_reply_t *rep, int pid, int tid)
{
    char fifostr[USER_FIFO_PATH_LEN];
    sprintf(fifostr, "%s%05d", USER_FIFO_PATH_PREFIX, pid);

    int repFifo;
    if ((repFifo = open(fifostr, O_WRONLY | O_NONBLOCK)) == -1)
    {
        rep->value.header.ret_code = RC_USR_DOWN;
        return;
    }

    write(repFifo, rep, sizeof(tlv_reply_t));

    close(repFifo);
}

void *thr_open_office(void *arg)
{
    int tid = *(int *)arg;
    int semValue;
    logBankOfficeOpen(logFd, tid, pthread_self());

    while (!srvShutdown)
    {

        // waits full semaphore
        sem_getvalue(&full, &semValue);
        logSyncMechSem(logFd, tid, SYNC_OP_SEM_WAIT, SYNC_ROLE_CONSUMER, 0, semValue);
        sem_wait(&full);

        // mutex lock
        pthread_mutex_lock(&queueAccessMutex);
        logSyncMech(logFd, tid, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_CONSUMER, 0);

        tlv_request_t req;
        tlv_reply_t rep;

        // so that in shutdown, the requests in queue are processed
        while (!isEmptyQueue(&q))
        {

            dequeue(&q, &req);

            // mutex unlock
            pthread_mutex_unlock(&queueAccessMutex);
            logSyncMech(logFd, tid, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_CONSUMER, req.value.header.pid);

            logRequest(logFd, tid, &req);

            // locks accounts access
            pthread_mutex_lock(&accountsAccessMutex);
            logSyncMech(logFd, tid, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_ACCOUNT, req.value.header.pid);

            //takes care of request, filling reply struct
            processRequest(&req, &rep, tid);

            // unlocks accounts access
            pthread_mutex_unlock(&accountsAccessMutex);
            logSyncMech(logFd, tid, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_ACCOUNT, req.value.header.pid);

            // sends reply to user
            sendReply(&rep, req.value.header.pid, tid);
            logReply(logFd, tid, &rep);

            // posts empty semaphore
            sem_post(&empty);
            sem_getvalue(&empty, &semValue);
            logSyncMechSem(logFd, tid, SYNC_OP_SEM_POST, SYNC_ROLE_CONSUMER, req.value.header.pid, semValue);

            // preparing next iteration of processing, garanteeing that the queue gets emptied
            // waits full semaphore
            sem_getvalue(&full, &semValue);
            logSyncMechSem(logFd, tid, SYNC_OP_SEM_WAIT, SYNC_ROLE_CONSUMER, 0, semValue);
            sem_wait(&full);

            // mutex lock
            pthread_mutex_lock(&queueAccessMutex);
            logSyncMech(logFd, tid, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_CONSUMER, 0);
        }

        // mutex unlock CHANGE TO SEMAPHORE???
        pthread_mutex_unlock(&queueAccessMutex);
        logSyncMech(logFd, tid, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_CONSUMER, 0);

        // posts empty semaphore
        sem_post(&empty);
        sem_getvalue(&empty, &semValue);
        logSyncMechSem(logFd, tid, SYNC_OP_SEM_POST, SYNC_ROLE_CONSUMER, 0, semValue);
    }

    logBankOfficeClose(logFd, tid, pthread_self());

    return NULL;
}

int main(int argc, char **argv)
{

    srand(time(NULL));

    if (argumentHandler(argc, argv)) // handles the arguments and creates admin account
        exit(1);

    threadNum = atoi(argv[1]);

    // creates and/or opens server log file with rw-rw-rw- permissions
    logFd = open(SERVER_LOGFILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (logFd == -1)
    {
        write(STDERR_FILENO, "Server Log File could not be opened\n", 37);
        exit(1);
    }
    else
    {
        on_exit(closeFd, &logFd);
    }

    // locks acc mutex
    pthread_mutex_lock(&accountsAccessMutex);
    logSyncMech(logFd, MAIN_THREAD_ID, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_ACCOUNT, ADMIN_ACCOUNT_ID);

    //makes admin acc
    bank_account_t adminAcc;
    createAdminAccount(argv[2], &adminAcc);
    logAccountCreation(logFd, MAIN_THREAD_ID, &adminAcc);

    //logs delay
    logSyncDelay(logFd, MAIN_THREAD_ID, ADMIN_ACCOUNT_ID, 0);

    //unlocks acc mutex
    pthread_mutex_unlock(&accountsAccessMutex);
    logSyncMech(logFd, MAIN_THREAD_ID, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_ACCOUNT, ADMIN_ACCOUNT_ID);

    // creates and opens FIFO to communication user->server with rw-rw-rw- permissions
    if (mkfifo(SERVER_FIFO_PATH, 0666) != 0)
    {
        write(STDERR_FILENO, "FIFO tmp/secure_srv could not be created successfully\n", 55);
        exit(1);
    }
    else
        on_exit(removePath, SERVER_FIFO_PATH);

    fifoFd = open(SERVER_FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fifoFd == -1)
    {
        write(STDERR_FILENO, "FIFO tmp/secure_srv could not be opened\n", 41);
        exit(1);
    }
    else
    {
        on_exit(closeFd, &fifoFd);
    }

    // init of Full semaphore
    logSyncMechSem(logFd, MAIN_THREAD_ID, SYNC_OP_SEM_INIT, SYNC_ROLE_PRODUCER, 0, 0);
    sem_init(&full, NOT_SHARED, 0);

    // init of Full semaphore
    logSyncMechSem(logFd, MAIN_THREAD_ID, SYNC_OP_SEM_INIT, SYNC_ROLE_PRODUCER, 0, threadNum);
    sem_init(&empty, NOT_SHARED, threadNum);

    // creation of threads
    pthread_t threads[threadNum];
    int tid[threadNum];
    for (int i = 0; i < threadNum; i++)
    {
        tid[i] = i + 1;
        pthread_create(&threads[i], NULL, thr_open_office, &tid[i]);
    }

    // initialization of queue
    queueInit(&q, sizeof(tlv_request_t));
    emptyQueue = 0; // stores the number of elements of the queue
    int semValue;

    // waits for request on fifo and when reads, put in queue
    while (!srvShutdown)
    {
        // reads request
        tlv_request_t req;
        while (read(fifoFd, &req, sizeof(tlv_request_t)) > 0)
        {
            logRequest(logFd, MAIN_THREAD_ID, &req);

            // waits empty semaphore
            sem_getvalue(&empty, &semValue);
            logSyncMechSem(logFd, MAIN_THREAD_ID, SYNC_OP_SEM_WAIT, SYNC_ROLE_PRODUCER, req.value.header.pid, semValue);
            sem_wait(&empty);

            // locks exclusive access mutex
            pthread_mutex_lock(&queueAccessMutex);
            logSyncMech(logFd, MAIN_THREAD_ID, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_PRODUCER, req.value.header.pid);

            // puts received req in queue
            enqueue(&q, &req);
            emptyQueue++;

            // unlocks exclusive access mutex
            pthread_mutex_unlock(&queueAccessMutex);
            logSyncMech(logFd, MAIN_THREAD_ID, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_PRODUCER, req.value.header.pid);

            // posts full semaphore
            sem_post(&full);
            sem_getvalue(&full, &semValue);
            logSyncMechSem(logFd, MAIN_THREAD_ID, SYNC_OP_SEM_POST, SYNC_ROLE_PRODUCER, req.value.header.pid, semValue);
        }
    }

    for (int i = 0; i < threadNum; i++)
    {
        sem_post(&full);
    }

    for (int i = 0; i < threadNum; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&accountsAccessMutex);
    pthread_mutex_destroy(&queueAccessMutex);
    sem_destroy(&full);
    sem_destroy(&empty);

    return 0;
}
