#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    setenv("TERM", "linux", 1);
    setenv("USER", "root", 1);
    setenv("HOME", "/root", 1);
    setenv("PATH", "/usr/local/bin:/usr/bin:/usr/sbin", 1);

    const char* path = "/usr/bin/bash";
    if (access(path, X_OK) == -1)
    {
        perror("init: failed to access shell");
        return EXIT_FAILURE;
    }

    puts("\e[2JWelcome to CryptixOS!\n");
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
