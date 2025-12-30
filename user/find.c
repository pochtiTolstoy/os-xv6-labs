#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define BUFFER_SIZE 512

#define REQUIRED_NUM_ARGS 3

struct fs_entry
{
    int fd;
    struct stat st;
    const char *path;
};

#if 0
static void process_file(const struct fs_entry *entry, const char *file_to_find);
static void process_file(const struct fs_entry *entry, const char *file_to_find)
{
    fprintf(STDOUT, "[LOG]: TODO %s\n", __func__);
    exit(1);
}
#endif

static void process_dir(const struct fs_entry *entry, const char *file_to_find);

int main(int argc, char* argv[])
{
    // TODO: implement --help flag
    if (argc == 2 && strcmp(argv[1], "--help") == 0)
    {
        fprintf(STDOUT, "find: help page\n");
        fprintf(STDOUT, "----------------------------------------\n");
        fprintf(STDOUT, "usage:   find [directory] [file to find]\n");
        fprintf(STDOUT, "example: find . file.txt\n");
        exit(0);
    }

    if (argc != REQUIRED_NUM_ARGS)
    {
        fprintf(STDERR, "find: wrong number of arguments: %d\n", argc);
        fprintf(STDERR, "Try 'find --help' for more information.\n");
        exit(1);
    }

    const char *find_path = argv[1];
    const char *file_to_find = argv[2];

    struct fs_entry entry = { .path = find_path };

    if ((entry.fd = open(entry.path, 0)) < 0)
    {
        fprintf(STDERR, "find: cannot open %s\n", entry.path);
        close(entry.fd);
        exit(1);
    }

    if (fstat(entry.fd, &entry.st) < 0)
    {
        fprintf(STDERR, "find: cannot stat %s\n", entry.path);
        close(entry.fd);
        exit(1);
    }

    switch (entry.st.type)
    {
        case T_DEVICE:
        case T_FILE:
            fprintf(STDERR, "find: expected path as 1st argument: %s\n", entry.path);
            fprintf(STDERR, "Try 'find --help' for more information.\n");
            break;
        case T_DIR:
            process_dir(&entry, file_to_find);
            break;
        default:
            fprintf(STDERR, "find: unknown file type %hd\n", entry.st.type);
            close(entry.fd);
            exit(1);
    }

    close(entry.fd);
    exit(0);
}

static void process_dir(const struct fs_entry *entry, const char *file_to_find)
{
    struct dirent de;
    char answer_buffer[BUFFER_SIZE] = {};

    fprintf(STDOUT, "[LOG]: %s\n", __func__);

    while (read(entry->fd, &de, sizeof(de)) == sizeof(de))
    {
        if (de.inum == 0)
        {
            continue;
        }
        fprintf(STDOUT, "[LOG]: read dir %s : %s\n", entry->path, de.name);

        // without recursion:
        char file_name[DIRSIZ + 1];
        memmove(file_name, de.name, DIRSIZ);
        file_name[DIRSIZ] = 0;

        if (strcmp(file_to_find, file_name) == 0)
        {
            char *write_iterator = answer_buffer;

            strcpy(write_iterator, entry->path);
            write_iterator += strlen(entry->path);

            strcpy(write_iterator, "/");
            write_iterator += 1;

            strcpy(write_iterator, file_name);
        }
    }

    if (answer_buffer[0])
    {
        fprintf(STDOUT, "%s\n", answer_buffer);
    }
    else
    {
        fprintf(STDOUT, "can't find %s in %s directory.\n", file_to_find, entry->path);
    }
}