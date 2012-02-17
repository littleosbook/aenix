#include "unistd.h"
#include "string.h"
#include "sys/syscall.h"

int main(void)
{
    int i = 0, j = 0;
    if (syscall(SYS_fork)) {
        while (1) {
            syscall(SYS_write, 1, "parent\n", 7);
            while (j++ < 1000000) {} /* do work */
            j = 0;

            if (i++ >= 10) {
                char *msg = "parent doing nothing\n";
                syscall(SYS_write, 1, msg, strlen(msg));
                syscall(SYS_wait);
                syscall(SYS_write, 1, "done waiting\n", 13);

                while (1) {
                }
            }
        }
    } else {
        while (1) {
            syscall(SYS_write, 1, "child\n", 6);
            while (j++ < 1000000) {} /* do work */
            j = 0;

            if (i++ >= 7) {
                syscall(SYS_write, 1, "child exiting\n", 14);
                return 0;
            }
        }
    }

    return 0;
}
