#include <stdio.h>
#include <sys/reboot.h>
#include <linux/reboot.h>

int main()
{
    printf("Rebooting...");
    reboot(0);

    return -1;
}
