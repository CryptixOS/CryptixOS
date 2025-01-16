#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

void prompt(void)
{
    const char* prompt_str = ("cryptixos@root> ");

    write(0, prompt_str, strlen(prompt_str));
}

typedef struct
{
    char*  buffer;
    size_t size;
} string_t;

void resize(string_t* str)
{
    char* newBuffer = malloc(str->size * 2);
    memcpy(newBuffer, str->buffer, str->size);
    free(str->buffer);
    str->buffer = newBuffer;
    str->size *= 2;
}

void run_cmd(string_t* binary, size_t size)
{
    binary->buffer[size] = 0;

    int pid              = fork();

    if (pid == 0)
    {
        const char* path   = binary->buffer;
        const char* args[] = {path, 0};
        const char* env[]  = {0};

        execve(path, (char* const*)args, (char* const*)env);
    }
}

int main()
{
    prompt();

    string_t str;
    str.buffer   = malloc(32);
    str.size     = 32;
    size_t index = 0;

    for (;;)
    {
        char* buf = readline("\n>>> ");
        write(0, buf, 5);
        char    keybuf[16];

        ssize_t nread = read(0, keybuf, sizeof(keybuf));

        for (ssize_t i = 0; i < nread; i++)
        {
            if (index >= str.size) resize(&str);

            write(0, &keybuf[i], 1);
            if (keybuf[i] == '\b')
            {
                index--;
                continue;
            }
            if (keybuf[i] == '\n')
            {
                run_cmd(&str, index);
                index = 0;
                continue;
            }

            str.buffer[index] = keybuf[i];

            index++;
        }
    }
}
