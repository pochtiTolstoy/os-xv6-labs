#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define BUFFER_SIZE 512

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

    const char *find_path = "./test_dir";
    const char *file_to_find = "test_file_a";

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
        if (strcmp(file_to_find, de.name) == 0)
        {
            char *write_iterator = answer_buffer;

            strcpy(write_iterator, entry->path);
            write_iterator += strlen(entry->path);

            strcpy(write_iterator, "/");
            write_iterator += 1;

            strcpy(write_iterator, de.name);

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