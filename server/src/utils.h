#ifndef UTILS_H
#define UTILS_H

#include "sope.h"

int argumentHandler(int argc, char **argv);

void closeFd(int r, void* arg); 

void removePath(int r, void* arg); 

#endif 
