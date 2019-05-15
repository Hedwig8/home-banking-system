#ifndef DATAMANIP_H
#define DATAMANIP_H

#include "authentication.h"
#include "utils.h"

void createAdminAccount(char password[], bank_account_t* adminAcc);

int findAccount(int id, bank_account_t *acc);

int createAccount(req_create_account_t req, tlv_reply_t *rep);

void getBalance(req_header_t req, tlv_reply_t *rep);

int consumeTransfer(req_header_t src, req_transfer_t dest, tlv_reply_t *rep);



#endif
