#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "dataManip.h"
#include "queue.h"
#include "utils.h"

// current number of threads
int threadNum, logFd;
Queue q;
bool srvShutdown = false;
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;

void processRequest (tlv_request_t *req, tlv_reply_t *rep) {

    bool verification = true;
    bank_account_t acc;

    /**
     *  MUTEX
     *  LOCK
     * 
     */


    // check if exists
    if(findAccount(req->value.header.account_id, &acc)) {
        rep->value.header.ret_code = RC_ID_NOT_FOUND;
        verification = false;
    }

    // check if pair (id, password) is correct
    if(checkLogin(req->value.header.password, acc)) {
        rep->value.header.ret_code = RC_LOGIN_FAIL;
        verification = false;
    }

    rep->type = req->type;
    rep->value.header.account_id = req->value.header.account_id;

    // if error occured, prepare reply and return function
    if(!verification) {
        switch(req->type) {
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
        /**
         * 
         * MUTEX
         * UNLOCK
         * 
         */
        return;
    }


    switch (req->type)
    {
    case OP_CREATE_ACCOUNT:
        // must be admin
        if(acc.account_id != 0) {
            rep->value.header.ret_code = RC_OP_NALLOW;
            break;
        }

        // acc must not exist
        if(createAccount(req->value.create)) {
            rep->value.header.ret_code = RC_ID_IN_USE;
            break;
        }

        break;

    case OP_BALANCE:
        // cannot be admin
        if(acc.account_id == 0) {
            rep->value.header.ret_code = RC_OP_NALLOW;
            break;
        }


        break;

    case OP_TRANSFER:
        // cannot be admin
        if(acc.account_id == 0) {
            rep->value.header.ret_code = RC_OP_NALLOW;
            break;
        }
        break;

    case OP_SHUTDOWN:
        // must be admin
        if(acc.account_id != 0) {
            rep->value.header.ret_code = RC_OP_NALLOW;
            break;
        }

        srvShutdown = true;
        break;

    default:
        break;
    }


}

void sendReply(tlv_reply_t *rep) {

}

void *thr_fifo_answer(void *arg)
{
    int tid = *(int *)arg;
    logBankOfficeOpen(logFd, tid, pthread_self());


    while(!srvShutdown) {
        //pthread_mutex_lock(&queueMutex);
        //logSyncMech(logFd, tid, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_CONSUMER, 0);
        if(!isEmptyQueue(&q)) {
            tlv_request_t req;
            tlv_reply_t rep;
            dequeue(&q, &req);
            //pthread_mutex_unlock (&queueMutex);
            //logSyncMech(logFd, tid, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_CONSUMER, req.value.header.pid);
            
            //fuc para tratar do pedido
            logRequest(STDOUT_FILENO, tid, &req);
            processRequest(&req, &rep);
            sendReply(&req);
            //srvShutdown = true;
        }
        //pthread_mutex_unlock(&queueMutex);
        //logSyncMech(logFd, tid, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_CONSUMER, 0);
    }

    logBankOfficeClose(logFd, tid, pthread_self());
}




int main(int argc, char **argv)
{

    srand(time(NULL));

    if (argumentHandler(argc, argv)) // handles the arguments and creates admin account
        exit(1);


    // creates and opens FIFO to communication user->server
    if(mkfifo(SERVER_FIFO_PATH, 0666) != 0)
    {
        write(STDERR_FILENO, "FIFO tmp/secure_srv could not be created successfully\n", 55);
        exit(1);
    } 
    else 
        on_exit(removePath, SERVER_FIFO_PATH);

    int fifoFd= open(SERVER_FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fifoFd == -1) {
        write(STDERR_FILENO, "FIFO tmp/secure_srv could not be opened\n",41);
        exit(1);
    }
    else
    {
        on_exit(closeFd, &fifoFd);
    }
    
    // creates and/or opens server log file 
    logFd = open(SERVER_LOGFILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(logFd == -1) {
        write(STDERR_FILENO, "Server Log File could not be opened\n", 37);
        exit(1);
    }
    else {
        on_exit(closeFd, &logFd);
    }


    // creation of threads
    pthread_t threads[threadNum];
    int tid[threadNum];
    for (int i = 0; i < threadNum; i++)
    {
        tid[i] = i;
        pthread_create(&threads[i], NULL, thr_fifo_answer, &tid[i]);
    }

    // initialization of queue
    queueInit(&q, sizeof(tlv_request_t));

    // waits for request on fifo and when reads, put in queue
    while (!srvShutdown)
    {
        // reads request
        tlv_request_t req;
        if (read(fifoFd, &req, sizeof(tlv_request_t)) > 0)
        {
            logRequest(logFd, MAIN_THREAD_ID, &req);
            //pthread_mutex_lock(&queueMutex);
            //logSyncMech(logFd, MAIN_THREAD_ID, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_PRODUCER, req.value.header.pid);
           

            enqueue(&q, &req);

            //pthread_mutex_unlock(&queueMutex);
            //logSyncMech(logFd, MAIN_THREAD_ID, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_PRODUCER, req.value.header.pid);
        }
    }

    //pthread_mutex_destroy (queueMutex);

    for (int i = 0; i < threadNum; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}