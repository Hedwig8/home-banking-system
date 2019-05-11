

//provided code
#include "sope.h"


int argumentHandler(int argc, char ** argv) {
    if(argc < 1 || argc > 6) {
        write(STDOUT_FILENO, "usage: user ..........", 23);
        return 1;
    }


    return 0;
}


int main(int argc, char ** argv) {

    if (argumentHandler(argc, argv)) return 1;

    return 0;
}