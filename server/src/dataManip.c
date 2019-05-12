#include "dataManip.h"

// array of MAX_BANK_ACCOUNTS
static bank_account_t accounts[MAX_BANK_ACCOUNTS] = {0};

void createAdminAccount(char password[]) {

    bank_account_t adminAcc;
    adminAcc.account_id = ADMIN_ACCOUNT_ID;
    adminAcc.balance = 0;

    getSaltKey(adminAcc.salt);
    calculateHash(password, adminAcc.salt, adminAcc.hash);

    accounts[0] = adminAcc;

    logAccountCreation(STDOUT_FILENO, 0, &adminAcc);
}

int createAccount(bank_account_t acc) {

}