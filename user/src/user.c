#include <stdlib.h>
#include <string.h>

//provided code
#include "sope.h"

//int opDelay;

int argumentHandler(int argc, char * argv[]) {
    if(argc != 6) {
        write(STDERR_FILENO, "usage: user <account ID> <password> <op delay> <op> <op args>\n", 63);
        write(STDERR_FILENO, "  op: 0 - create acc; 1 - balance inquiry; 2 - transfer; 3 - server closing\n", 77);
        write(STDERR_FILENO, "    op 0 require as argument \"<new_account_id> <initial balance> <password>\"\n", 78);
        write(STDERR_FILENO, "    op 2 require as argument \"<destiny_account_ID> <amount>\"\n", 62);
        return 1;
    }



    //opDelay = atoi(argv[3]);

    return 0;
}

int requestPackagePrep(char * argv[], tlv_request_t *req) {
    req->type = atoi(argv[4]);

    req->value.header.account_id = atoi(argv[1]);
    req->value.header.op_delay_ms = atoi(argv[3]);
    strcpy(req->value.header.password,argv[2]);
    req->value.header.pid = getpid();

    char id[WIDTH_ID], password[MAX_PASSWORD_LEN], balance[WIDTH_BALANCE], amount[WIDTH_BALANCE];
    switch(req->type) {
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
}

int main(int argc, char * argv[]) {

    if (argumentHandler(argc, argv)) return 1;

    tlv_request_t request;
    requestPackagePrep(argv, &request);

    return 0;
}