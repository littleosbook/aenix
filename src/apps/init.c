#include "unistd.h"
#include "string.h"
#include "sys/syscall.h"

int main(void)
{
    int pid;

    /* open the devices for standard file descriptors */
    syscall(SYS_open, "/dev/keyboard"); /* STDIN  */
    syscall(SYS_open, "/dev/console");  /* STDOUT */
    syscall(SYS_open, "/dev/console");  /* STDERR */

    /* start the shell */
    pid = syscall(SYS_fork);
    if (pid == 0) {
        syscall(SYS_execve, "/bin/sh");
    } else {
        while (syscall(SYS_wait) != -1) {
        }
        /* TODO: shut down the kernel */
        while (1) {}
    }

    return 0;
}
