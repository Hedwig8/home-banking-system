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

    if (0 > atoi(argv[1]) || atoi(argv[1]) >= MAX_BANK_ACCOUNTS)
    {
        write(STDERR_FILENO, "account ID must be a valid number [0-4096]\n", 44);
        return 1;
    }

    if (MIN_PASSWORD_LEN > strlen(argv[2]) || strlen(argv[2]) > MAX_PASSWORD_LEN)
    {
        write(STDERR_FILENO, "password must have a valid length [8-20]\n", 42);
        return 1;
    }

    if (0 > atoi(argv[3]) || atoi(argv[3]) > MAX_OP_DELAY_MS)
    {
        write(STDERR_FILENO, "delay must be a valid number [0-99999]\n", 44);
        return 1;
    }

    char password[MAX_PASSWORD_LEN + 1];
    char *garbageStr = calloc(sizeof(char), MAX_PASSWORD_LEN + 1);
    int balance;
    int accountID;

    switch (atoi(argv[4]))
    {
    case 1:
        sscanf(argv[5], "%s", garbageStr);
        if (strcmp("", garbageStr) != 0)
        {
            write(STDERR_FILENO, "op 1 should not have arguments: \"\" \n", 38);
            return 1;
        }
        break;

    case 3:
        sscanf(argv[5], "%s", garbageStr);
        if (strcmp("", garbageStr) != 0)
        {
            write(STDERR_FILENO, "op 3 should not have arguments: \"\" \n", 38);
            return 1;
        }
        break;

    case 2:
        sscanf(argv[5], "%d %d%s", &accountID, &balance, garbageStr);
        if (0 >= accountID || accountID >= MAX_BANK_ACCOUNTS)
        {
            write(STDERR_FILENO, "destiny account ID must be a valid number [1-4095]\n", 52);
            return 1;
        }
        if (0 > balance || balance > MAX_BALANCE)
        {
            write(STDERR_FILENO, "amount must be a valid number [1-1000000000]\n", 46);
            return 1;
        }
        if (strcmp(garbageStr, "") != 0)
        {
            write(STDERR_FILENO, "op 2 should not have more than 2 arguments\n", 44);
            return 1;
        }
        break;

    case 0:
        sscanf(argv[5], "%d %d %s %s", &accountID, &balance, password, garbageStr);
        if (0 >= accountID || accountID >= MAX_BANK_ACCOUNTS)
        {
            write(STDERR_FILENO, "new account ID must be a valid number [1-4096]\n", 48);
            return 1;
        }
        if (0 > balance || balance > MAX_BALANCE)
        {
            write(STDERR_FILENO, "initial balance must be a valid number [1-1000000000]\n", 55);
            return 1;
        }
        //test password
        if ((strlen(password) < MIN_PASSWORD_LEN) || (strlen(password) > MAX_PASSWORD_LEN))
        {
            write(STDERR_FILENO, "password must have a valid length [8-20]\n", 42);
            return 1;
        }

        if (strcmp(garbageStr, "") != 0)
        {
            write(STDERR_FILENO, "op 0 should not have more than 3 arguments\n", 44);
            return 1;
        }
        break;

    default:
        write(STDERR_FILENO, "usage: user <account ID> <password> <op delay> <op> <op args>\n", 63);
        write(STDERR_FILENO, "  op: 0 - create acc; 1 - balance inquiry; 2 - transfer; 3 - server closing\n", 77);
        write(STDERR_FILENO, "    op 0 require as argument \"<new_account_id> <initial balance> <password>\"\n", 78);
        write(STDERR_FILENO, "    op 2 require as argument \"<destiny_account_ID> <amount>\"\n", 62);
        return 1;
    }

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
        strcpy(id, strtok(argv[5], " "));
        strcpy(balance ,strtok(NULL, " "));
        strcpy(password, strtok(NULL, " "));
        req->value.create.account_id = atoi(id);
        strcpy(req->value.create.password, password);
        req->value.create.balance = atoi(balance);
        break;
    case OP_BALANCE:
        break;
    case OP_TRANSFER:
        strcpy(id, strtok(argv[5], " "));
        strcpy(amount, strtok(NULL, " "));
        req->value.transfer.account_id = atoi(id);
        req->value.transfer.amount = atoi(amount);
        break;
    case OP_SHUTDOWN:
        break;
    case __OP_MAX_NUMBER:
        break;
    }

    req->length = sizeof(req->value);
    return 0;
}

void closeFd(int r, void *arg)
{
    close(*(int *)arg);
}

void removeFifo(int r, void *arg)
{
    unlink((char *)arg);
}

void replyErrorPackagePrep(enum ret_code errorCode, tlv_reply_t *reply, tlv_request_t *request)
{
    reply->type = request->type;
    reply->value.header.account_id = request->value.header.account_id;
    reply->value.header.ret_code = errorCode;

    switch (reply->type)
    {
    case OP_BALANCE:
        reply->value.balance.balance = 0;
        break;
    case OP_TRANSFER:
        reply->value.transfer.balance = 0;
        break;
    case OP_SHUTDOWN:
        reply->value.shutdown.active_offices = 0;
        break;
    default:
        break;
    }

    reply->length = sizeof(reply->value);
}

int main(int argc, char *argv[])
{
    tlv_reply_t reply;
    tlv_request_t request;
    int timer = FIFO_TIMEOUT_SECS;
    bool successRead = false;

    // check arguments validity
    if (argumentHandler(argc, argv))
        return 1;

    // complete request with args
    requestPackagePrep(argv, &request);

    // opens ulog.txt
    int logFd = open(USER_LOGFILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
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
        replyErrorPackagePrep(RC_SRV_DOWN, &reply, &request);

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
    if (mkfifo(fifostr, 0666) != 0)
    {
        replyErrorPackagePrep(RC_OTHER, &reply, &request);

        logReply(logFd, getpid(), &reply);

        write(STDERR_FILENO, "fifo was not created successfully\n", 35);
        exit(1);
    }
    else
        on_exit(removeFifo, fifostr);

    int repFifo = open(fifostr, O_RDONLY | O_NONBLOCK);

    // error opening
    if (repFifo == -1)
    { //if could not open, closes

        replyErrorPackagePrep(RC_OTHER, &reply, &request);

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

    // to make sure that the log is done
    while (logRequest(logFd, getpid(), &request) == -1)
    {
    }

    // 30 seconds loop
    while (timer && !successRead)
    {
        sleep(1);
        timer--;
        if (read(repFifo, &reply, sizeof(tlv_reply_t)) > 0)
            successRead = true;
    }

    if (!successRead)
    {
        replyErrorPackagePrep(RC_SRV_TIMEOUT, &reply, &request);
    }

    while (logReply(logFd, getpid(), &reply) == -1)
    {
    }

    exit(0);
}