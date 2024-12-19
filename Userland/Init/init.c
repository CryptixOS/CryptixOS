#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/mman.h>
#include <unistd.h>

void prompt(void)
{
    const char* prompt_str = ("cryptixos@root> ");

    write(0, prompt_str, strlen(prompt_str));
}

int main()
{
    prompt();
    char*       keyBuffer    = mmap(0, 4096, 0x03, MAP_ANONYMOUS, -1, 0);
    int         pid          = fork();
    const char* child        = "child";
    const char* fork_fail    = "fork-failed";
    const char* fork_success = "fork-success";

    if (pid == 0) { write(0, child, 5); }
    else if (pid == -1) { write(0, fork_fail, 11); }
    else write(0, fork_success, 12);

    for (;;)
    {
        ssize_t nread;
        nread = getline(&keyBuffer, &nread, stdin);

        if (nread > 0) write(0, keyBuffer, nread);
    }
}
