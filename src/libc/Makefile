CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
		 -nostartfiles -nodefaultlibs -Wall -Wextra -Werror \
		 -Wno-unused-function -c
AS = nasm
ASFLAGS = -f elf
OBJECTS = unistd.o start.o string.o

all: libc.a

libc.a: $(OBJECTS)
	ar rcs libc.a $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -rf *.a *.o
