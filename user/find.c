#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "user/user.h"

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define REQUIRED_NUM_ARGS 3

static void display_help(void);
static void concat_paths(char *new_path, const char *dir_path, const char *entry_name);
static int process_dir(const char *dir_path, const char *file_to_find);

int main(int argc, char* argv[])
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0)
    {
        display_help();
        exit(0);
    }

    // Validate number of arguments
    if (argc != REQUIRED_NUM_ARGS)
    {
        fprintf(STDERR, "find: wrong number of arguments: %d\n", argc);
        fprintf(STDERR, "Try 'find --help' for more information.\n");
        exit(1);
    }

    const char *working_path = argv[1];
    const char *file_to_find = argv[2];

    struct stat st;
    if (stat(working_path, &st) < 0)
    {
        fprintf(STDERR, "find: cannot stat path: %s\n", working_path);
        exit(1);
    }

    if (st.type != T_DIR)
    {
        fprintf(STDERR, "find: given path is not a directory: %s\n", working_path);
        fprintf(STDERR, "Try 'find --help' for more information.\n");
        exit(1);
    } 
    
    int err = process_dir(working_path, file_to_find);

    exit(err);
}

static void display_help(void) 
{
    fprintf(STDOUT, "find: help page\n");
    fprintf(STDOUT, "----------------------------------------\n");
    fprintf(STDOUT, "usage:   find [directory] [file to find]\n");
    fprintf(STDOUT, "example: find . file.txt\n");
}

static int process_dir(const char *dir_path, const char *file_to_find)
{
    char new_path[MAXPATH + 1] = {};
    struct dirent de;
    struct stat st;
    int fd;
    int recv; // bytes read
    int had_error = 0; // Track if any error occurred
    int dir_path_len = strlen(dir_path);

    // Open given directory
    if ((fd = open(dir_path, 0))< 0)
    {
        fprintf(STDERR, "find: cannot open directory: %s\n", dir_path);
        return 1;
    }

    // Read directory entries
    while ((recv = read(fd, &de, sizeof(de))) == sizeof(de))
    {
        if (de.inum == 0)
            continue;

        // Get entry name
        char entry_name[DIRSIZ + 1] = {};
        memmove(entry_name, de.name, DIRSIZ);
        entry_name[DIRSIZ] = 0;
        int entry_name_len = strlen(entry_name);

        if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0)
            continue;

        if (dir_path_len + 1 + entry_name_len > MAXPATH)
        {
            fprintf(STDERR, "find: path too long: %s/%s\n", dir_path, entry_name);
            continue;
        }

        // Concat dir_path and entry_name to form new_path
        concat_paths(new_path, dir_path, entry_name);

        // Stat the new path
        if (stat(new_path, &st) < 0)
        {
            fprintf(STDERR, "find: cannot stat file: %s\n", new_path);
            had_error = 1;
            continue;
        }

        // Process based on type
        switch (st.type) {
            case T_FILE:
                if (strcmp(entry_name, file_to_find) == 0) {
                    fprintf(STDOUT, "%s\n", new_path);
                }
                break;
            case T_DIR:
                if (process_dir(new_path, file_to_find) != 0)
                    had_error = 1; // Propagate error from recursive call
                break;
            default:
                break;
        }
    }

    if (recv < 0)
    {
        fprintf(STDERR, "find: error reading directory: %s\n", dir_path);
        had_error = 1;
    }

    close(fd);

    return had_error;
}

static void concat_paths(char *new_path, const char *dir_path, const char *entry_name)
{
    int dir_path_len = strlen(dir_path);
    int entry_name_len = strlen(entry_name);

    char *output_iter = new_path;

    // Copy dir_path
    strcpy(output_iter, dir_path);
    output_iter += dir_path_len;

    // Add '/'
    *output_iter++ = '/';

    // Copy entry_name
    strcpy(output_iter, entry_name);
    output_iter += entry_name_len;

    // Null-terminate
    *output_iter = '\0';
}