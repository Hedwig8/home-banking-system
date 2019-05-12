#ifndef PASSWORDSALT_H
#define PASSWORDSALT_H

#include <stdlib.h>
#include <string.h>

#include "sope.h"


int getSaltKey(char *oSalt);

int calculateHash(char* iPassword, char* salt, char* oHash);

#endif
