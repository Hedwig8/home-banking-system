#include "dataManip.h"

// array of MAX_BANK_ACCOUNTS
static bank_account_t accounts[MAX_BANK_ACCOUNTS] = {0};

void createAdminAccount(char password[], bank_account_t *adminAcc) {

    adminAcc->account_id = ADMIN_ACCOUNT_ID;
    adminAcc->balance = 0;

    //char *auxstr = calloc(SALT_LEN, sizeof(char));
    //getSaltKey(auxstr);
    strcpy(adminAcc->salt, getSaltKey());
    calculateHash(password, adminAcc->salt, adminAcc->hash);

    accounts[0] = *adminAcc;
}

int findAccount(int id, bank_account_t *acc) {
    *acc = accounts[id];

    if(id != 0 && acc->account_id == 0) return 1;

    return 0;
}

int createAccount(req_create_account_t req, tlv_reply_t *rep) {
    bank_account_t acc;

    // id must not exist
    if(findAccount(req.account_id, &acc) == 0) {
        rep->value.header.ret_code = RC_ID_IN_USE;
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
    rep->value.header.ret_code = RC_OK;
    return 0;
}

void getBalance(req_header_t req, tlv_reply_t* rep) {
    bank_account_t acc = accounts[req.account_id];

    rep->value.balance.balance = acc.balance;
    rep->value.header.ret_code = RC_OK;
}

int consumeTransfer(req_header_t src, req_transfer_t dest, tlv_reply_t *rep) {
    bank_account_t srcAcc, destAcc;

    // check if src acc exists
    if(findAccount(src.account_id, &srcAcc)) {
        rep->value.header.ret_code = RC_ID_NOT_FOUND;
        rep->value.transfer.balance = 0;
        return 1;
    }

    // check if dest acc exists
    if(findAccount(dest.account_id, &destAcc)) {
        rep->value.header.ret_code = RC_ID_NOT_FOUND;
        rep->value.transfer.balance = srcAcc.balance;
        return 1;
    }

    // check if accs are not the same
    if(srcAcc.account_id == destAcc.account_id) {
        rep->value.header.ret_code = RC_SAME_ID;
        rep->value.transfer.balance = srcAcc.balance;
        return 1;
    }

    // check if src acc has enough funds to complete transfer
    if(srcAcc.balance < dest.amount) { 
        rep->value.header.ret_code = RC_NO_FUNDS;
        rep->value.transfer.balance = srcAcc.balance;
        return 1;
    }

    // check if dest acc with transfer doesnt surpass the limit
    if((destAcc.balance + dest.amount) > MAX_BALANCE ) {
        rep->value.header.ret_code = RC_TOO_HIGH;
        rep->value.transfer.balance = srcAcc.balance;
        return 1;
    }

    // execute the transfer
    srcAcc.balance -= dest.amount;
    destAcc.balance += dest.amount;

    // updates acc in array
    accounts[srcAcc.account_id] = srcAcc;
    accounts[destAcc.account_id] = destAcc;

    rep->value.header.ret_code = RC_OK;
    rep->value.transfer.balance = srcAcc.balance;
    return 0;
}
