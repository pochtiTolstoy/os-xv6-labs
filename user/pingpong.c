#include "kernel/types.h"
#include "user/user.h"

#define BYTE 69

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define PIPE_READ_END  0
#define PIPE_WRITE_END 1

void check_byte_warn(char byte1, char byte2);

int main(int argc, char *argv[])
{
    char bytes[] = { BYTE };

    // pipe[0] - read end of pipe
    // pipe[1] - write end of pipe
    int parent_pipe[2] = {};
    pipe(parent_pipe);

    int child_pipe[2] = {};
    pipe(child_pipe);

    if (fork() == 0) 
    {
        // child process

        // close fds
        close(parent_pipe[PIPE_WRITE_END]);
        close(child_pipe[PIPE_READ_END]);

        // recv byte via parent pipe
        char recv_bytes[] = { 0 };
        read(parent_pipe[PIPE_READ_END], recv_bytes, sizeof(recv_bytes));
        close(parent_pipe[PIPE_READ_END]);
        check_byte_warn(recv_bytes[0], bytes[0]);

        // print info
        int child_pid = getpid();
        fprintf(STDOUT, "%d: received ping\n", child_pid, recv_bytes[0]);

        // send byte to parent via child pipe
        write(child_pipe[PIPE_WRITE_END], bytes, sizeof(bytes));
        close(child_pipe[PIPE_WRITE_END]);

        // child should exit
        exit(0);
    }

    // close fds
    close(parent_pipe[PIPE_READ_END]);
    close(child_pipe[PIPE_WRITE_END]);

    // parent process
    write(parent_pipe[PIPE_WRITE_END], bytes, sizeof(bytes));
    close(parent_pipe[PIPE_WRITE_END]);

    // read from child process
    char recv_bytes[] = { 0 };
    read(child_pipe[PIPE_READ_END], recv_bytes, sizeof(recv_bytes));
    close(child_pipe[PIPE_READ_END]);
    check_byte_warn(recv_bytes[0], bytes[0]);

    // print info
    int parent_pid = getpid();
    fprintf(STDOUT, "%d: received pong\n", parent_pid, recv_bytes[0]);

    // wait for child
    wait(0);
    exit(0);
}

void check_byte_warn(char byte1, char byte2)
{
    if (byte1 != byte2) 
    {
        fprintf(STDOUT, "warning: expected %d byte, received %d\n", byte1, byte2);
    }
}