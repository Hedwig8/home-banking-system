#ifndef DATAMANIP_H
#define DATAMANIP_H

#include "passwordSalt.h"

void createAdminAccount(char password[]);

int createAccount(bank_account_t acc);

int findAccount(int id, bank_account_t *acc);

int consumeTransfer(bank_account_t* src, bank_account_t dest);

#endif
