CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
		 -nostartfiles -nodefaultlibs -Wall -Wextra -Werror \
		 -Wno-unused-function -c
AS = nasm
ASFLAGS = -f elf
LD = ld
LDFLAGS = -T link.ld -melf_i386
LIBC = -L../libc -lc
STDINCLUDE = -I../libc

all: init sh

init: init.o
	$(LD) $(LDFLAGS) init.o $(LIBC) -o init

sh: sh.o
	$(LD) $(LDFLAGS) sh.o $(LIBC) -o sh

%.o: %.c
	$(CC) $(CFLAGS) $(STDINCLUDE) $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -rf init *.o
