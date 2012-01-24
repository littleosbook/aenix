# Makefile for the kernel. Based on the makefile found at
# http://www.jamesmolloy.co.uk/tutorial_html/1.-Environment%20setup.html

OBJECTS = loader.o kmain.o fb.o io.o gdt.o gdt_asm.o pic.o idt.o idt_asm.o \
		  interrupt.o interrupt_asm.o keyboard.o pit.o stdio.o string.o \
		  paging.o paging_asm.o
MODULES = 
CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
		 -nostartfiles -nodefaultlibs -Wall -Wextra -Werror
LDFLAGS = -T link.ld -melf_i386
AS = nasm
ASFLAGS = -f elf

all: kernel.elf

run: iso
	./run_bochs.sh

iso: kernel.elf
	./create_iso.sh $(MODULES)

kernel.elf: $(OBJECTS)
	ld $(LDFLAGS) $(OBJECTS) -o kernel.elf

.o: 
	$(CC) $(CFLAGS) $< -o $@

.s.o: 
	$(AS) $(ASFLAGS) $< 

doc:
	cd .. ; doxygen doc/Doxyfile

clean:
	rm -rf *.o kernel.elf aenix.iso
