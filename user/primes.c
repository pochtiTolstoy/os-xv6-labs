#include "kernel/types.h"
#include "user/user.h"

#define START_NUMBER 2
#define END_NUMBER   35

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define READ_END  0
#define WRITE_END 1

static void start_pipeline_node(int [], int []);

int main(int argc, char *argv[])
{
    // sender process
    int sender_pipe[2] = {};
    if (pipe(sender_pipe) < 0)
    {
        fprintf(STDERR, "error: pipe() failed\n");
        exit(1);
    }

    start_pipeline_node(sender_pipe, 0);
    close(sender_pipe[READ_END]);

    for (uint number = START_NUMBER; number <= END_NUMBER; ++number)
    {
        // at this point parent has only WRITE end of this pipe
        write(sender_pipe[WRITE_END], &number, sizeof(number));
    }

    close(sender_pipe[WRITE_END]);

    wait(0);
    exit(0);
}

/*
 * start_pipeline_node() - creates new process to print prime number and filter others.
 *
 * Resource management correctness:
 *
 *   Suppose we have node n[x], this node will have recv_pipe and trash_pipe via previous call
 *   of this function. trash_pipe[W] is closed, because n[x-1] node clears it's recv_pipe[W]
 *   before calls to start_pipeline_node. Both fds of recv_pipe is opened, because of
 *   n[x-1] sender_pipe passed to start_pipeline_node.
 *
 *   So, it responsible for 5 fds after node creation:
 *   1. trash_pipe[R] is closed immediately.
 *   2. recv_pipe[W]  is closed immediately.
 *   3. recv_pipe[R] is closed at the end of process after fork 
 *      (it will affect new process n[x+1] thats why we need trash_pipe).
 *   4-5. In the middle n[x] creates sender_pipe[R/W] and forwards it to new node n[x+1],
 *      n[x] will close sender_pipe[R/W] at the end.
 *
 */
static void start_pipeline_node(int recv_pipe[], int trash_pipe[]) 
{
    if (fork() != 0) 
    {
        // parent process:
        return;
    }

    // child process:

    uint base_number = 0;
    uint recv_number = 0;
    int is_node_created = 0;
    int sender_pipe[2] = {};

    if (trash_pipe != 0)
    {
        close(trash_pipe[READ_END]);
    }

    close(recv_pipe[WRITE_END]);

    if (pipe(sender_pipe) < 0)
    {
        fprintf(STDERR, "error: pipe() failed\n");
        exit(1);
    }

    // first received number by process - is prime number
    if (read(recv_pipe[READ_END], &base_number, sizeof(base_number)) != sizeof(base_number))
    {
        fprintf(STDERR, "error: read() failed\n");
        exit(1);
    }
    fprintf(STDOUT, "prime %d\n", (int)base_number);

    while (read(recv_pipe[READ_END], &recv_number, sizeof(recv_number)) == sizeof(recv_number))
    {
        if (recv_number % base_number != 0)
        {
            if (!is_node_created)
            {
                start_pipeline_node(sender_pipe, recv_pipe);
                is_node_created = 1;
            }
            write(sender_pipe[WRITE_END], &recv_number, sizeof(recv_number));
        }
    }

    close(recv_pipe[READ_END]);
    close(sender_pipe[WRITE_END]);
    close(sender_pipe[READ_END]);

    wait(0);
    exit(0);
}