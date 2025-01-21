#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    setenv("TERM", "linux", 1);
    setenv("USER", "root", 1);
    setenv("HOME", "/root", 1);
    setenv("PATH", "/usr/local/bin:/usr/bin", 1);

    const char* path = "/usr/bin/bash";
    if (access(path, X_OK) == -1)
    {
        perror("init: failed to access shell");
        return EXIT_FAILURE;
    }

    for (;;)
    {
        printf("Welcome to CryptixOS!");

        int pid = fork();
        if (pid == -1)
        {
            perror("init: fork failed");
            return EXIT_FAILURE;
        }
        else if (pid == 0)
        {
            char* const argv[] = {(char*)path, NULL};
            chdir(getenv("HOME"));
            execvp(path, argv);
            return EXIT_FAILURE;
        }

        int status;
        waitpid(pid, &status, 0);
    }

    return EXIT_SUCCESS;
}
