#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    setenv("TERM", "linux", 1);
    setenv("USER", "root", 1);
    setenv("HOME", "/root", 1);

    const char* path = "/usr/bin/bash";
    if (access(path, X_OK) == -1)
    {
        perror("init: failed to access shell");
        return EXIT_FAILURE;
    }

    while (1)
    {
        int pid = fork();
        if (pid == -1)
        {
            perror("init: fork failed");
            return EXIT_FAILURE;
        }
        else if (pid == 0)
        {
            char* argv[] = {path, NULL};
            chdir(getenv("HOME"));
            execvp(path, argv);
            return EXIT_FAILURE;
        }

        waitpid(pid, NULL, 0);
        break;
    }

    return EXIT_SUCCESS;
}
