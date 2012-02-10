#include "unistd.h"
#include "string.h"
#include "sys/syscall.h"

int main(void)
{
    char *aenix =
"=======================================================\n"
"       d8888 8888888888 888b    888 8888888 Y88b   d88P\n"
"      d88888 888        8888b   888   888    Y88b d88P \n"
"     d88P888 888        88888b  888   888     Y88o88P  \n"
"    d88P 888 8888888    888Y88b 888   888      Y888P   \n"
"   d88P  888 888        888 Y88b888   888      d888b   \n"
"  d88P   888 888        888  Y88888   888     d88888b  \n"
" d8888888888 888        888   Y8888   888    d88P Y88b \n"
"d88P     888 8888888888 888    Y888 8888888 d88P   Y88b\n"
"=======================================================\n";
    syscall(SYS_open, "/dev/console");
    syscall(SYS_write, 0, aenix, strlen(aenix));
    char *str = "\n";
    syscall(SYS_write, 0, str, strlen(str));
    syscall(SYS_write, 0, str, strlen(str));
    str = "aenix> ";
    syscall(SYS_write, 0, str, strlen(str));

    while(1) {

    }

    return 0;
}
