#include <errno.h>
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
    char* keyBuffer = mmap(0, 4096, 0x03, MAP_ANONYMOUS, -1, 0);

    for (;;)
    {
        char    keybuf[16];

        ssize_t nread = read(0, keybuf, sizeof(keybuf));
        if (nread < 0 && errno == EINTR) return 2;

        for (ssize_t i = 0; i < nread; ++i)
        {
            putchar(keybuf[i]);
            fflush(stdout);
        }
        // if (nread > 0) write(0, keyBuffer, nread);
    }
}
