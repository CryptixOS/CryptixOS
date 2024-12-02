#include <stdint.h>
#include <stdio.h>

#include <sys/mman.h>
#include <unistd.h>

void prompt(void) { printf("cryptixos@root> "); }

int  main()
{
    prompt();
    char* keyBuffer = mmap(0, 4096, 0x03, MAP_ANONYMOUS, -1, 0);

    for (;;)
    {
        size_t nread = read(0, keyBuffer, 16);

        // if (nread) write(0, keyBuffer, nread);
    }
}
