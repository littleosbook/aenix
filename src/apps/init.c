#include "unistd.h"
#include "string.h"
#include "sys/syscall.h"

static void print_banner()
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
    syscall(SYS_write, 0, aenix, strlen(aenix));
}

static void println()
{
    syscall(SYS_write, 0, "\n", 1);
}

int main(void)
{
    syscall(SYS_open, "/dev/console");
    syscall(SYS_open, "/dev/keyboard");

    print_banner();
    println();
    println();

    char *prompt = "aenix> ";
    while(1) {
        syscall(SYS_write, 0, prompt, strlen(prompt));

        char ch = 0, buf[128];
        size_t i = 0;
        while (ch != '\n' && i < 127) {
            syscall(SYS_read, 1, &ch, 1);
            buf[i] = ch;
            ++i;
        }
        buf[i] = '\0';
        syscall(SYS_write, 0, buf, i);
    }

    return 0;
}
