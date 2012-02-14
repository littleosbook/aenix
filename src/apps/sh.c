#include "unistd.h"
#include "string.h"
#include "sys/syscall.h"

int main(void)
{
    if (syscall(SYS_fork)) {
        while (1) {
            syscall(SYS_write, 1, "parent\n", 7);
            syscall(SYS_yield);
        }
    } else {
        while (1) {
            syscall(SYS_write, 1, "child\n", 6);
            syscall(SYS_yield);
        }
    }

    return 0;
}
