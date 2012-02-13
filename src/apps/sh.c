#include "unistd.h"
#include "string.h"
#include "sys/syscall.h"

int main(void)
{
    char *prompt = "aenix> ";

    while(1) {
        syscall(SYS_write, 1, prompt, strlen(prompt));

        char ch = 0, buf[128];
        size_t i = 0;
        while (ch != '\n' && i < 127) {
            syscall(SYS_read, 0, &ch, 1);
            buf[i] = ch;
            ++i;
        }
        buf[i] = '\0';
        syscall(SYS_write, 1, buf, i);
    }

    return 0;
}
