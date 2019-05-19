#include "utils.h"

int argumentHandler(int argc, char **argv)
{
    if (argc != 3)
    {
        write(STDERR_FILENO, "usage: server <no of bank offices> <admin password>\n", 53);
        return 1;
    }

    int threadNum = atoi(argv[1]);
    if (threadNum <= 0 || threadNum > MAX_BANK_OFFICES)
    {
        write(STDERR_FILENO, "range of bank offices must be [1-99]\n", 38);
        return 1;
    }

    if (strlen(argv[2]) < MIN_PASSWORD_LEN || strlen(argv[2]) > MAX_PASSWORD_LEN)
    {
        write(STDERR_FILENO, "password length must be [8-20] characters\n", 43);
        return 1;
    }
    return 0;
}

void closeFd(int r, void* arg)
{
    close(*(int*)arg);
}

void removePath(int r, void* arg) 
{
    unlink((char *) arg);
}