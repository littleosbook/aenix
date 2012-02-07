#include "unistd.h"
#include "sys/syscall.h"

int main(void)
{
    syscall(SYS_write);
    syscall(SYS_write);

    while(1) {

    }

    return 0;
}
