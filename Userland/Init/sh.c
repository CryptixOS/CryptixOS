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

    char linebuf[128];
    int  linedx = 0;
    linebuf[0]  = '\0';

    for (;;)
    {
        char    keybuf[16];

        ssize_t nread = read(0, keybuf, sizeof(keybuf));
        // if (nread < 0 && errno == EINTR) return 2;

        for (ssize_t i = 0; i < nread; ++i)
        {
            write(0, keybuf + i, 1);
            fflush(stdout);
            if (keybuf[i] != '\n')
            {
                linebuf[linedx++] = keybuf[i];
                linebuf[linedx]   = '\0';
            }
            else
            {
                linebuf[0] = '\0';
                linedx     = 0;
                prompt();
            }
        }
    }
}
