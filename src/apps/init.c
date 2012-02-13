#include "unistd.h"
#include "string.h"
#include "sys/syscall.h"

int main(void)
{
    /* open the devices for standard file descriptors */
    syscall(SYS_open, "/dev/keyboard"); /* STDIN  */
    syscall(SYS_open, "/dev/console");  /* STDOUT */
    syscall(SYS_open, "/dev/console");  /* STDERR */

    /* start the shell */
    syscall(SYS_execve, "/bin/sh");

    return 0;
}
