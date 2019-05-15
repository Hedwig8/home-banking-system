#ifndef DATAMANIP_H
#define DATAMANIP_H

#include "authentication.h"
#include "utils.h"

void createAdminAccount(char password[]);

int findAccount(int id, bank_account_t *acc);

int createAccount(req_create_account_t req);

int consumeTransfer(req_header_t src, req_transfer_t dest, tlv_reply_t *rep);

#endif
