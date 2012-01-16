# Makefile for the kernel. Based on the makefile found at
# http://www.jamesmolloy.co.uk/tutorial_html/1.-Environment%20setup.html

OBJECTS = loader.o kmain.o
CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles \
		 -nodefaultlibs -Wall -Wextra -c
LDFLAGS = -T link.ld -melf_i386
AS = nasm
ASFLAGS = -f elf

all: kernel

run: floppy
	cd ../tools; ./run_bochs.sh

floppy: kernel
	./create_floppy.sh

kernel: $(OBJECTS)
	ld $(LDFLAGS) $(OBJECTS) -o kernel

.o: 
	$(CC) $(CFLAGS) $< -o $@

.s.o: 
	$(AS) $(ASFLAGS) $< 

clean:
	rm -r *.o kernel bin
