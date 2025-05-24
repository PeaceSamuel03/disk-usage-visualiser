#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#define GREEN "\033[1;36m"
#define BLUE "\033[1;34m"
#define RESET "\033[0m"

typedef struct
{
    char name[NAME_MAX];
    char path[PATH_MAX];
    __off_t size;
    int is_dir;
} FileEntry;

int compare_by_size(const void *a, const void *b)
{
    FileEntry *fa = (FileEntry *)a;
    FileEntry *fb = (FileEntry *)b;
    return (fb->size - fa->size);
    // compare function for qsort (descending by size)
}

int compare_by_name(const void *a, const void *b)
{
    FileEntry *fa = (FileEntry *)a;
    FileEntry *fb = (FileEntry *)b;
    return strcmp(fa->name, fb->name);
    //(ascending by name)
}

// helper function to convert file sizes from bytes to KB/MB
void format_size(__off_t size_bytes, char *output, size_t max_len)
{
    if (size_bytes < 1024)
    {
        snprintf(output, max_len, "%ld B", size_bytes);
    }
    else if (size_bytes < 1024 * 1024)
    {
        snprintf(output, max_len, "%.2f KB", size_bytes / 1024.0);
    }
    else if (size_bytes < 1024 * 1024 * 1024)
    {
        snprintf(output, max_len, "%.2f MB", size_bytes / (1024.0 * 1024));
    }
    else
    {
        snprintf(output, max_len, "%.2f GB", size_bytes / (1024.0 * 1024 * 1024));
    }
}

// write a recursive function that traverses directories, add sorting
__off_t traverse_directory(const char *dir_path, int depth, int sort_by_size, int show_hidden)
{
    // calculate total size of dir
    __off_t total_size = 0;

    // takes a dir path, visits each entry in it, if it finds a subdir, it recursively calls itself
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
    {
        // print error if it can't be opened
        perror("opendir");
        return 0;
    }

    // to read dir contents
    struct dirent *entry;
    int capacity = 10, count = 0;
    FileEntry *entries = malloc(sizeof(FileEntry) * capacity);

    while ((entry = readdir(dir)) != NULL)
    {
        if (!show_hidden && entry->d_name[0] == '.')
        {
            continue;
        }
        // read each entry (file & folder)
        // skip "." and ".." to avoid infinite recursion
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // build full path to the entry
        char path[4096];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        struct stat info;
        if (stat(path, &info) != 0)
        {
            perror("stat");
            continue;
        }

        if (count >= capacity)
        {
            capacity *= 2;
            entries = realloc(entries, sizeof(FileEntry) * capacity);
            if (!entries)
            {
                perror("realloc");
                closedir(dir);
                return 0;
            }
        }

        FileEntry fe;
        strncpy(fe.name, entry->d_name, NAME_MAX);
        strncpy(fe.path, path, PATH_MAX);
        fe.size = info.st_size;
        fe.is_dir = S_ISDIR(info.st_mode);
        entries[count++] = fe;
    }

    closedir(dir);

    // sort files by size
    qsort(entries, count, sizeof(FileEntry), sort_by_size ? compare_by_size : compare_by_name);

    for (int i = 0; i < count; i++)
    {
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entries[i].name);

        // indent output based on depth in dir tree
        for (int i = 0; i < depth; i++)
        {
            printf("  ");
        }

        // use stat to find out if its a dir, metadata about the path to find out if its a dir-> stat(path, &info) == 0 && S_ISDIR(info.st_mode)
        if (entries[i].is_dir)
        {
            // its a dir so call recursively
            // print in color code
            // check if its a hidden dir
            if (entries[i].name[0] == '.')
            {
                printf("\033[38;5;129m[DIR] %s\033[0m\n", entries[i].name);
            }
            else
            {
                printf("\033[1;34m[DIR] %s\033[0m\n", entries[i].name);
            }
            total_size += traverse_directory(full_path, depth + 1, sort_by_size, show_hidden);
        }
        else
        {
            // its a regular file, add color code
            // printf("%s (%lld bytes)\n", entry->d_name, (long long)info.st_size);
            char size_str[32];
            format_size(entries[i].size, size_str, sizeof(size_str));
            // check for hidden file
            if (entries[i].name[0] == '.')
            {
                printf("\033[1;35m%s (%s)\033[0m\n", entries[i].name, size_str);
            }
            else
            {
                printf("\033[1;36m%s (%s)\033[0m\n", entries[i].name, size_str);
            }
            total_size += entries[i].size;
        }
    }

    // after visiting all entries, print total for the folder
    for (int i = 0; i < depth; i++)
    {
        printf(" ");
    }
    char total_str[32];
    format_size(total_size, total_str, sizeof(total_str));
    printf("==> Total size of %s: %s\n", dir_path, total_str);
    free(entries);
    return total_size;
}

int main(int argc, char *argv[])
{
    int sort_by_size = 1;
    int show_hidden = 0;
    // use "." if not path given
    const char *start_path = ".";
    //(argc > 1) ? argv[1] : ".";
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-s") == 0)
        {
            sort_by_size = 1;
        }
        else if (strcmp(argv[i], "-n") == 0)
        {
            sort_by_size = 0;
        }
        else if (strcmp(argv[i], "-h") == 0)
        {
            show_hidden = 1;
        }
        else
        {
            start_path = argv[i];
        }
    }

    printf("Directory structure of : %s\n\n", start_path);
    traverse_directory(start_path, 0, sort_by_size, show_hidden);
    return 0;
}
////////////////////////////////////////////////////////////////
// indent output based on depth in dir tree
// for (int i = 0; i < depth; i++)
// {
//     printf("  ");
// }

// // use stat to find out if its a dir, metadata about the path to find out if its a dir
// if (stat(path, &info) == 0 && S_ISDIR(info.st_mode))
// {
//     // its a dir so call recursively
//     // print in color code
//     printf("BLUE[DIR] %s\033[0m\n", entry->d_name);
//     total_size += traverse_directory(path, depth + 1);
// }
// else if (S_ISREG(info.st_mode))
// {
//     // its a regular file, add color code
//     // printf("%s (%lld bytes)\n", entry->d_name, (long long)info.st_size);
//     char size_str[32];
//     format_size(info.st_size, size_str, sizeof(size_str));
//     printf("GREEN%s (%s)\033[0m\n", entry->d_name, size_str);
//     total_size += info.st_size;
// }
// else
// {
//     // other files types
//     printf("%s [special file]\n", entry->d_name);
// }