#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define ARGSIZE 128  // max length of each exec argument

// #define MAXARG 32  // max exec arguments

// #define LOG

static void free_memory(char **exec_args, int exec_argc);

int main(int argc, char *argv[]) 
{
    // If no initial arguments, use default program "echo"
    char default_program[] = "echo";
    if (argc == 1)
    {
        // No initial arguments, use default program
        argc = 2;
        argv[1] = default_program;
    }

    // Validate number of initial arguments
    int user_argc = argc - 1;
    int pipe_argc = 1; // each line from stdin is one argument
    int exec_argc_capacity = user_argc + pipe_argc;
    if (exec_argc_capacity > MAXARG)
    {
        fprintf(STDERR, "xargs: too many initial arguments\n");
        exit(EXIT_FAILURE);
    }

    int recv;
    char ch;
    int exec_argc = 0;
    int end_iter = 0;
    char **exec_args = malloc(sizeof(char *) * (exec_argc_capacity + 1));
    if (exec_args == 0)
    {
        fprintf(STDERR, "xargs: memory allocation failure\n");
        exit(EXIT_FAILURE);
    }
    exec_args[exec_argc_capacity] = 0; // null-terminate for exec

    // Copy initial arguments
    for (int i = 0; i < user_argc; ++i)
    {
        exec_args[exec_argc] = malloc(sizeof(char) * (strlen(argv[i + 1]) + 1));
        if (exec_args[exec_argc] == 0)
        {
            fprintf(STDERR, "xargs: memory allocation failure\n");
            goto failure;
        }
        strcpy(exec_args[exec_argc], argv[i + 1]);
        exec_args[exec_argc][strlen(argv[i + 1])] = 0;
        ++exec_argc;
    }

    const char *user_program = exec_args[0];

    // Read from stdin
    char buffer[ARGSIZE];
    while ((recv = read(STDIN, &ch, sizeof(ch))) > 0)
    {
        if (end_iter >= sizeof(buffer) - 1)
        {
            fprintf(STDERR, "xargs: argument too long\n");
            goto failure;
        }
        buffer[end_iter++] = ch;

        // On newline, finalize the argument, and run exec
        if (ch == '\n')
        {
            buffer[--end_iter] = 0; // replace '\n' with '\0'
            exec_args[exec_argc] = malloc(end_iter + 1);
            if (exec_args[exec_argc] == 0)
            {
                fprintf(STDERR, "xargs: memory allocation failure\n");
                goto failure;
            }
            strcpy(exec_args[exec_argc], buffer);
            exec_args[exec_argc][end_iter] = 0;
            end_iter = 0;

            // Exec program with arguments from exec_args
            if (fork() == 0)
            {
                exec(user_program, exec_args); // skip program name
                fprintf(STDERR, "xargs: exec %s failed\n", user_program);
                goto failure;
            }
            else
            {
                int wstatus;
                wait(&wstatus);
                if (wstatus != 0)
                {
                    fprintf(STDERR, "xargs: child process exited with status %d\n", wstatus);
                    goto failure;
                }
            }

            free(exec_args[exec_argc]); // free previous argument before reusing the slot
        }
    }       

    if (recv < 0)
    {
        fprintf(STDERR, "xargs: error reading from stdin\n");
        goto failure;
    }

    free_memory(exec_args, exec_argc);
    exit(EXIT_SUCCESS);

failure:
    free_memory(exec_args, exec_argc);
    exit(EXIT_FAILURE);
}

static void free_memory(char **exec_args, int exec_argc)
{
    for (int i = 0; i < exec_argc; ++i)
    {
        free(exec_args[i]);
    }
    free(exec_args);
}