#include "dataManip.h"

// array of MAX_BANK_ACCOUNTS
static bank_account_t accounts[MAX_BANK_ACCOUNTS] = {0};

void createAdminAccount(char password[]) {

    bank_account_t adminAcc;
    adminAcc.account_id = ADMIN_ACCOUNT_ID;
    adminAcc.balance = 0;

    //char *auxstr = calloc(SALT_LEN, sizeof(char));
    //getSaltKey(auxstr);
    strcpy(adminAcc.salt, getSaltKey());
    calculateHash(password, adminAcc.salt, adminAcc.hash);

    accounts[0] = adminAcc;

    logAccountCreation(STDOUT_FILENO, 0, &adminAcc);
}

int findAccount(int id, bank_account_t *acc) {
    *acc = accounts[id];

    if(acc == 0) return 1;

    return 0;
}

//int createAccount(bank_account_t acc) {
int createAccount(req_create_account_t req) {
    bank_account_t acc;
    
    // id must not exist
    if(findAccount(req.account_id, &acc) == 0) {
        return 1;
    }

    // fills acc
    acc.account_id = req.account_id;
    acc.balance = req.balance;
    strcpy(acc.salt, getSaltKey());
    // calculates hash
    char hash[HASH_LEN];
    calculateHash(req.password, acc.salt, hash);
    strcpy(acc.hash, hash);

    //writes to array
    accounts[acc.account_id] = acc;
    return 0;
}

int consumeTransfer(req_header_t src, req_transfer_t dest, tlv_reply_t *rep) {

}
