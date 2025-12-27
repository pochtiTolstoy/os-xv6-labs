* Solution for make qemu stuck while booting: [link](https://github.com/mit-pdos/xv6-riscv/issues/262)
* Command to clone: `git clone git://g.csail.mit.edu/xv6-labs-2022`
### Useful info:
* to run: `make qemu`
* xv6 has no ps command, but, if you type `Ctrl-p`, the kernel will print information about each process. If you try it now, you'll see two lines: one for init, and one for sh.
* To quit qemu type: `Ctrl-a x` (press Ctrl and a at the same time, followed by x). 

### Book chapter 1:
When a process needs to invoke a *kernel service*, it invokes a *system call*.  
The kernel uses the *hardware protection* provided by a *CPU* to ensure that each process executing in user space can access only its own memory.  
The kernel executes with the *hardware privileges* required to implement these protections.  

**General idea**: When a user program invokes a *system-call*, the *hardware* raises the *privilege level* and starts executing a pre-arragned function in the *kernel*.  

#### 1.1 Processes and memory:
* User-space memory: instr, data, stack  
* Kernel-space memory: private to kernel  

xv6 - *time-shares* processes: switch the available CPUs among the set of processes waiting to execute.  

When a process is *not executing*, xv6 saves the process's *CPU registers*, restoring them when it next runs the process.  

The kernel *associates* a process id, or *PID*, with each process.  

A process may create a new process using the `fork` system call, it just copy everything from old process to new one: *instr, data, stack*.  

In the *parent* process returns *child* process's *PID*, in new process, fork returns *0*.  

If the caller has no children, wait immediately returns -1.  

The `exec` system call replaces the calling process's memory with a new memory image loaded from a file stored in the file system.  

The xv6 uses *ELF* format for binary. The instr loaded from the file start executing at the *entry point* declared in the ELF header.  

A process that needs more memory at run-time can call `sbrk` to grow its data memory by n zero bytes, sbrk returns the location of the *new memory*.  

**General idea**: fork creates new copy of process, you can easily detect child and parent by fork returned value. Usually system will use `fork + exec` combination in order to start new process.  

#### 1.2 I/O and File descriptors
A *file descriptor* is just integer for kernel-managed object.
Each process has table, fd is index in this table, each process has a private space of fds starting at zero.
* stdin - 0 (fd)
* stdout - 1 (fd)
* stderr - 2 (fd)

**General Rule**: a newly allocated fd is always the *lowest-numbered unused* descriptor of the current process.

It can be console or pipe, or file, or whatever.  

`fork` copies the parent's *fd table* along with its memory, so that the child starts with exactly the *same open files* as the parent.  

The system call `exec` replaces the calling process's memory but *preserves its file table*.  

This strange exception allows the shell to implement I/O redirection by forking, reopening chosen fds in the child, and then calling `exec` to run the new program:   
```C
char *argv[2];
argv[0] = "cat"; argv[1] = 0;
if (fork() == 0) {
    close(0);
    open("input.txt", O_RDONLY);
    exec("cat", argv);
}
```
After close - it releases 0 fd of current child process, next open is guarantee to return fd equal to 0 (according to **general rule**). Cat then executes with file descriptor 0. The parent process's fd are not changed by this sequence, since it modifies only the child's descriptors.

At this point it is clear why `fork` and `exec` are separate calls: between the two, the shell has a change to redirect the child's I/O without disturbing the I/O setup of the main shell.

`fork` copies the fd table, but file *offsets are shared* between parent and child.

The `dup`  system call *duplicates* an existing fd, returning a new one the refers to the same underlying I/O object. *Both share an offset!*
```C
fd = dup(1); // duplicate stdin
write(1, "hello ", 6);
write(fd, "world\n", 6); // shared offset -> "hello world"
```
So, we have transitivity in dup and fork operations in terms of shared offset between two fds.

fds do not share offsets, even if they resulted from open calls for the same I/O object.
`2>&1` can be implemented using `dup2(1, 2)`

**General idea:** general-rule, fd-table copy, fd-offset transitivity via fork, exec, dup, idea of redirection via `fork+close+open+exec`, dup can map fd to I/O object pointed by another fd.

#### 1.3 Pipes
Small kernel buffer - *pair of fds*, one for reading, one for writing.  
Program can close one side of pipe if its not needed.  
You can attach process fd to one end of pipe via same old trick with `fork + close + dup`  
Read on pipe will *wait for data*, or waits to all fds on write will be *closed*.  
Read will return 0 if all fds on write will be closed  
**General rule:** EOF on a pipe happens only when last write end is closed!  
If fd closed next exec will not see it even if it copies fd table.  
**General rule:** you need to think about which fds close before make exec for pipes  
**General idea:** pipes in unix made using kernel pipes, each leave - is command, each interior nodes are processes that wait until the left and right children complete.  
```C
int p[2];
char *argv[2];
argv[0] = "wc";
argv[1] = 0;
pipe(p);
if (fork() == 0) {
    close(0);
    dup(p[0]); // 0  -> p[0], this should be the only fd in exec fd table
    close(p[0]);
    close(p[1]);
    exec("/bin/wc", argv);
} else {
    close(p[0]);
    write(p[1], "hello world\n", 12);
    close(p[1]);
}
```

**Example:** `ls | wc`
```C
pipe(p); // create connection between p[0] and p[1]

if (fork() == 0) {        // child: ls
    dup2(p[1], 1);        // 1 -> obj(p[1])
    close(p[0]);
    close(p[1]);
    exec("ls");
}

if (fork() == 0) {        // child: wc
    dup2(p[0], 0);        // 0 -> obj(p[2])
    close(p[0]);
    close(p[1]);
    exec("wc");
}

// but parrent also should close pipes fds, in order to prevent bug with infinite wc read (because ref_count on write fds > 0)
close(p[0]);
close(p[1]);
wait(0);
wait(0);

// So in overall you have two fds: 
//         1 -> obj(p[1])
//         0 -> obj(p[0])
//         p[0] -> p[1] (pipe connection via kernell buffer)
```

**Advantages of pipes**:
* pipes automatically clean themselves up
* pipes can pass arbitrarily long streams of data
* pipes allow for parallel execution of pipeline stages

#### 1.4 File system
Process has *current directory*. You can change it via `chdir` system call.  

`mknod` creates a special file that refers to a device. Associated with a device file are the *major* and *minor* device numbers (two args to `mknod`), which uniquely identify a kernel device.  

File, called *inode*, can have multiple names, called *links*.  

*Entry* contains a *file name* and a reference to an *inode*.  

An inode holds *metadata* about a file, including its *type* (file or directory or device), its *length*, the *location* of the file's content on disk, and the *number of links* to a file.  

The `fstat` system call retrieves information from the inode that a file descriptor refers to. It fills in a *struct stat*  

The `link` system call creates another file system name referring to the same inode as an existing file.   


**Q:** how fd connected with *Entry*?

**Example**:
```C
open("a", O_CREATE|O_WRONLY);
link("a", "b");
```

Each inode is identified by a unique *inode number*.  

Fstat on same file (via links) will return the same *inode number (ino)*, and  the nlink count will be set to 2.  

The `unlink` system call removes a name from the file system. The file's inode and the disk space holding its content are only freed when the *file's link count is zero* and *no file descriptors* refer to it.  

You can create inode without name, just make open and unlink. Such inode will be cleaned up when the process closes fd or exits.  

**Exception:** `cd` must change the current working directory of the shell itself  

**General idea:** inodes and entries of a files are separated. Device in Unix has minor and major id's, which uniquely identifies it.  

#### 1.5 Real world
The Unix system call interface has been standardized through the *Portable Operating System Interface (POSIX)* standard. Xv6 is not POSIX compliant.  

**General idea:** all xv6 processes run as root.  


### Lab-1:
#### sleep:
```
Implement the UNIX program sleep for xv6; your sleep should pause for a user-specified number of ticks. A tick is a notion of time defined by the xv6 kernel, namely the time between two interrupts from the timer chip. Your solution should be in the file user/sleep.c. 

Some hints:
    * Before you start coding, read Chapter 1 of the xv6 book.
    * Look at some of the other programs in user/ (e.g., user/echo.c, user/grep.c, and user/rm.c) to see how you can obtain the command-line arguments passed to a program.
    * If the user forgets to pass an argument, sleep should print an error message.
    * The command-line argument is passed as a string; you can convert it to an integer using atoi (see user/ulib.c).
    * Use the system call sleep.
    * See kernel/sysproc.c for the xv6 kernel code that implements the sleep system call (look for sys_sleep), user/user.h for the C definition of sleep callable from a user program, and user/usys.S for the assembler code that jumps from user code into the kernel for sleep.
    * main should call exit(0) when it is done.
    * Add your sleep program to UPROGS in Makefile; once you've done that, make qemu will compile your program and you'll be able to run it from the xv6 shell.
    * Look at Kernighan and Ritchie's book The C programming language (second edition) (K&R) to learn about C. 
```
