#ifndef PASSWORDSALT_H
#define PASSWORDSALT_H

#include <stdlib.h>
#include <string.h>

#include "sope.h"


char * getSaltKey();

int calculateHash(char* iPassword, char* salt, char* oHash);

int checkLogin(char* password, bank_account_t acc);

#endif
