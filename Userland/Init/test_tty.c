#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

int main()
{
    struct termios old;
    //int tty = open("/dev/tty1", 0);
    int tty = 0;
    tcgetattr(tty, &old);

    struct termios n;
    memcpy(&n, &old, sizeof(struct termios));
    n.c_lflag &= ~ICANON;
    n.c_lflag &= ~ECHO;
    tcsetattr(tty, TCSANOW, &n);

    printf("\e[?1h");
    char c;
    bool escape = false;

    printf("START\n");
    while ((c = getchar()))
    {
        if (c == '\e')
            escape = true;
        else if (c == '\t') break;

        if (isprint(c)) printf("c: %c", c);
        else printf("c: %#x", c);
        printf("\n");
    }
    printf("END\n");

    printf("\e[?1l");
    tcsetattr(tty, TCSANOW, &old);
}
