#include "kernel/types.h"
#include "user/user.h"

#define REQUIRED_ARGS 2
#define HELP_FLAG_STR "--help"

void display_help();

int main(int argc, char *argv[])
{
    if (argc != REQUIRED_ARGS) 
    {
        printf("sleep: missing operand\n");
        printf("Try 'sleep --help' for more information.\n");
        exit(1);
    }

    if (strcmp(argv[1], HELP_FLAG_STR) == 0)
    {
        display_help();    
        exit(0);
    }

    int ticks = atoi(argv[1]);

    sleep(ticks); // system call

    exit(0);
}

void display_help()
{
    printf("sleep: help page\n");
    printf("usage: sleep [number of ticks to wait]\n");
}