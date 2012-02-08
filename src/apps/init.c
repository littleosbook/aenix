#include "unistd.h"
#include "string.h"
#include "sys/syscall.h"

int main(void)
{
    char *str = "hello, world!";
    syscall(SYS_write, 0, str, strlen(str));
    str = "goodbye!";
    syscall(SYS_write, 0, str, strlen(str));

    while(1) {

    }

    return 0;
}
