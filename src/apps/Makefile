CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
		 -nostartfiles -nodefaultlibs -Wall -Wextra -Werror \
		 -Wno-unused-function -c
AS = nasm
ASFLAGS = -f elf
LD = ld
LDFLAGS = -T link.ld
STDLIB = start.o interrupt.o

init: init.o $(STDLIB)
	$(LD) $(LDFLAGS) init.o $(STDLIB) -o init

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -rf init *.o
