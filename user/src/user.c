#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

//provided code
#include "sope.h"

//int opDelay;

int argumentHandler(int argc, char *argv[])
{
    if (argc != 6)
    {
        write(STDERR_FILENO, "usage: user <account ID> <password> <op delay> <op> <op args>\n", 63);
        write(STDERR_FILENO, "  op: 0 - create acc; 1 - balance inquiry; 2 - transfer; 3 - server closing\n", 77);
        write(STDERR_FILENO, "    op 0 require as argument \"<new_account_id> <initial balance> <password>\"\n", 78);
        write(STDERR_FILENO, "    op 2 require as argument \"<destiny_account_ID> <amount>\"\n", 62);
        return 1;
    }

    //opDelay = atoi(argv[3]);

    return 0;
}

int requestPackagePrep(char *argv[], tlv_request_t *req)
{
    req->type = atoi(argv[4]);

    req->value.header.account_id = atoi(argv[1]);
    req->value.header.op_delay_ms = atoi(argv[3]);
    strcpy(req->value.header.password, argv[2]);
    req->value.header.pid = getpid();

    char id[WIDTH_ID], password[MAX_PASSWORD_LEN], balance[WIDTH_BALANCE], amount[WIDTH_BALANCE];
    switch (req->type)
    {
    case OP_CREATE_ACCOUNT:
        strtok(argv[5], id);
        strtok(NULL, balance);
        strtok(NULL, password);
        req->value.create.account_id = atoi(id);
        strcpy(req->value.create.password, password);
        req->value.create.balance = atoi(balance);
        break;
    case OP_BALANCE:
        break;
    case OP_TRANSFER:
        strtok(argv[5], id);
        strtok(NULL, amount);
        req->value.transfer.account_id = atoi(id);
        req->value.transfer.amount = atoi(amount);
        break;
    case OP_SHUTDOWN:
        break;
    }

    req->length = sizeof(req->value);
}

void closeFd(int r, void *arg)
{
    close(*(int *)arg);
}

void removeFifo(int r, void * arg) {
    unlink((char*)arg);
}

int main(int argc, char *argv[])
{
    tlv_reply_t reply;
    tlv_request_t request;
    int timer = FIFO_TIMEOUT_SECS;
    bool successRead = false;


    // complete request with args
    requestPackagePrep(argv, &request);


    // check arguments validity
    if (argumentHandler(argc, argv))
        return 1;


    // opens ulog.txt
    int logFd = open(USER_LOGFILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    // error opening
    if (logFd == -1)
    {
        write(STDERR_FILENO, "logfile was not opened\n", 24);
        exit(1);
    }
    else
        on_exit(closeFd, &logFd);


    // opens user->server fifo
    int reqFifo = open(SERVER_FIFO_PATH, O_WRONLY | O_NONBLOCK);
    // error opening
    if (reqFifo == -1)
    { //if could not open, closes
        reply.type = request.type;
        reply.value.header.account_id = request.value.header.account_id;
        reply.value.header.ret_code = RC_SRV_DOWN;
        
        switch (reply.type)
        {
        case OP_BALANCE:
            reply.value.balance.balance = 0;
            break;
        case OP_TRANSFER:
            reply.value.transfer.balance = 0;
            break;
        case OP_SHUTDOWN:
            reply.value.shutdown.active_offices = 0;
            break;
        default:
            break;
        }

        reply.length = sizeof(reply.value);

        logReply(logFd, getpid(), &reply);

        write(STDERR_FILENO, "FIFO tmp/secure_srv couldnt be opened\n", 39);
        exit(1);
    }
    else
        on_exit(closeFd, &reqFifo);

 
    //opens server->user fifo
    char fifostr[USER_FIFO_PATH_LEN];
    sprintf(fifostr, "%s%05d", USER_FIFO_PATH_PREFIX, getpid());

    //creates fifo
    if(mkfifo(fifostr, 0666) != 0) {
        reply.type = request.type;
        reply.value.header.account_id = request.value.header.account_id;
        reply.value.header.ret_code = RC_OTHER;
        
        switch (reply.type)
        {
        case OP_BALANCE:
            reply.value.balance.balance = 0;
            break;
        case OP_TRANSFER:
            reply.value.transfer.balance = 0;
            break;
        case OP_SHUTDOWN:
            reply.value.shutdown.active_offices = 0;
            break;
        default:
            break;
        }

        reply.length = sizeof(reply.value);

        logReply(logFd, getpid(), &reply);

        write(STDERR_FILENO, "fifo was not created successfully\n", 35);
        exit(1);
    }
    else on_exit(removeFifo, fifostr);


    int repFifo = open(fifostr, O_RDONLY | O_NONBLOCK);

    // error opening
    if (repFifo == -1)
    { //if could not open, closes

        reply.type = request.type;
        reply.value.header.account_id = request.value.header.account_id;
        reply.value.header.ret_code = RC_OTHER;
        
        switch (reply.type)
        {
        case OP_BALANCE:
            reply.value.balance.balance = 0;
            break;
        case OP_TRANSFER:
            reply.value.transfer.balance = 0;
            break;
        case OP_SHUTDOWN:
            reply.value.shutdown.active_offices = 0;
            break;
        default:
            break;
        }

        reply.length = sizeof(reply.value);

        logReply(logFd, getpid(), &reply);

        char errstr[45];
        sprintf(errstr, "%s FIFO couldnt be opened\n", fifostr);
        write(STDERR_FILENO, errstr, strlen(errstr));
        exit(1);
    }
    else
        on_exit(closeFd, &repFifo);


    // sends request
    int size = write(reqFifo, &request, sizeof(tlv_request_t));
    if (size != sizeof(request))
    {
        write(STDERR_FILENO, "The request was not completely sent\n", 37);
    }
    printf("%d\n", size);

    // to make sure that the log is done
    while (logRequest(logFd, getpid(), &request) == -1) {}


    // 30 seconds loop
    while (timer && !successRead)
    {
        sleep(1);
        timer--;
        if (read(repFifo, &reply, sizeof(reply)) > 0)
            successRead = true;
    }

    if (!successRead)
    {
        reply.type = request.type;
        reply.value.header.account_id = request.value.header.account_id;
        reply.value.header.ret_code = RC_SRV_TIMEOUT;

        switch (reply.type)
        {
        case OP_BALANCE:
            reply.value.balance.balance = 0;
            break;
        case OP_TRANSFER:
            reply.value.transfer.balance = 0;
            break;
        case OP_SHUTDOWN:
            reply.value.shutdown.active_offices = 0;
            break;
        default:
            break;
        }
        reply.length = sizeof(reply.value);
    }

    logReply(logFd, getpid(), &reply);

    return 0;
}