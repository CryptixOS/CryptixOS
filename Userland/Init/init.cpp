#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 1024
#define MAX_FIELDS      6

struct MountPoint
{
    char* Source;         // Device or server
    char* Target;         // Mount point
    char* FilesystemType; // Filesystem type
    char* Options;        // Mount options
    int   Frequency;      // Dump frequency
    int   FsckPassNumber; // Fsck pass number
};

static size_t countOccurrences(const char* string, char c)
{
    size_t count = 0;
    while (*string++)
        if (*string == c) ++count;

    return count;
}

void trimNewLine(char* str)
{
    size_t len = strlen(str);
    if (len && str[len - 1] == '\n') str[len - 1] = '\0';
}
size_t parseMountOptions(char*& opts)
{
    size_t flags = 0;
    if (!opts) return 0;

    char*  options      = strdup(opts);
    char*  token        = strtok(options, ",");
    size_t optionLength = 0;

    char*  current      = opts;
    while (token)
    {
        if (!strcmp(token, "ro")) flags |= MS_RDONLY;
        else if (!strcmp(token, "noatime")) flags |= MS_NOATIME;
        else if (!strcmp(token, "relatime")) flags |= MS_RELATIME;
        else if (!strcmp(token, "nosuid")) flags |= MS_NOSUID;
        else if (!strcmp(token, "nodev")) flags |= MS_NODEV;
        else if (!strcmp(token, "noexec")) flags |= MS_NOEXEC;
        else if (!strcmp(token, "sync")) flags |= MS_SYNCHRONOUS;
        else if (!strcmp(token, "dirsync")) flags |= MS_DIRSYNC;
        else strcpy(current, token);

        current += strlen(token);
        token = strtok(NULL, ",");
    }

    delete options;
    *current = 0;
    return flags;
}

int parseMountPoint(const char* line, MountPoint& entry)
{
    char* fields[MAX_FIELDS] = {0};
    char* temp               = strdup(line);
    char* token              = strtok(temp, " \t");
    int   fieldCount         = 0;

    while (token && fieldCount < MAX_FIELDS)
    {
        fields[fieldCount++] = token;
        token                = strtok(NULL, " \t");
    }

    if (fieldCount < 4)
    {
        delete temp;
        return 0;
    }

    entry.Source         = strdup(fields[0]);
    entry.Target         = strdup(fields[1]);
    entry.FilesystemType = strdup(fields[2]);
    entry.Options        = strdup(fields[3]);
    entry.Frequency      = (fieldCount >= 5) ? atoi(fields[4]) : 0;
    entry.FsckPassNumber = (fieldCount >= 6) ? atoi(fields[5]) : 0;

    delete temp;
    return 1;
}

void freeMountPoint(MountPoint& entry)
{
    delete entry.Source;
    delete entry.Target;
    delete entry.FilesystemType;
    delete entry.Options;
}

int mountFStab()
{
    FILE* file = fopen("/etc/fstab", "r");
    if (!file)
    {
        perror("Failed to open /etc/fstab");
        return EXIT_FAILURE;
    }

    char line[MAX_LINE_LENGTH];
    int  lineNumber = 0;

    while (fgets(line, sizeof(line), file))
    {
        lineNumber++;
        trimNewLine(line);

        // Skip comments and empty lines
        if (line[0] == '#' || strlen(line) == 0) continue;

        MountPoint entry;
        if (parseMountPoint(line, entry))
        {
            size_t mountFlags = parseMountOptions(entry.Options);
            printf("options: %s\n", entry.Options);
            int mountStatus
                = mount(entry.Source, entry.Target, entry.FilesystemType,
                        mountFlags, entry.Options);
            printf("mount status: %d\n", mountStatus);
            if (mountStatus == -1)
                fprintf(stderr,
                        "init: failed to mount `%s` filesystem at `%s`, "
                        "source: %s, flags: `%s`\nerror code: %s\n",
                        entry.FilesystemType, entry.Target, entry.Source,
                        entry.Options, strerror(errno));

            freeMountPoint(entry);
        }
        else
            fprintf(stderr, "Skipping invalid or incomplete line %d\n",
                    lineNumber);
    }

    fclose(file);
    return EXIT_SUCCESS;
}

int main()
{
    setenv("TERM", "linux", 1);
    setenv("USER", "root", 1);
    setenv("HOME", "/root", 1);
    setenv("PATH", "/usr/local/bin:/usr/bin:/usr/sbin", 1);

    puts("\e[2JWelcome to CryptixOS!\n");
    if (mountFStab() != EXIT_SUCCESS)
        perror("init: failed to mount fstab entries");

    static const char* path = "/usr/bin/bash";
    if (access(path, X_OK) == -1)
    {
        perror("init: failed to access shell");
        return EXIT_FAILURE;
    }

    for (;;)
    {
        puts("init: launching shell...\n");

        int pid = fork();
        if (pid == -1)
        {
            perror("init: fork failed");
            return EXIT_FAILURE;
        }
        else if (pid == 0)
        {
            char* const argv[] = {(char*)path, "-i", NULL};
            chdir(getenv("HOME"));
            execvp(path, argv);
        }

        int status;
    continue_waiting:
        if (waitpid(pid, &status, 0) == pid)
        {
            bool exited = WIFEXITED(status);
            if (!exited) goto continue_waiting;

            printf("init: child %d died with exit code %d\n", pid,
                   WEXITSTATUS(status));
        }
    }

    return EXIT_SUCCESS;
}
