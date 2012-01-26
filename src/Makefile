OBJECTS = loader.o kmain.o fb.o io.o gdt.o gdt_asm.o pic.o idt.o idt_asm.o \
		  interrupt.o interrupt_asm.o keyboard.o pit.o stdio.o string.o \
		  paging.o paging_asm.o kmalloc.o module.o serial.o
MODULES = test_module.mod
CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
		 -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -Wno-unused-function
LDFLAGS = -T link.ld -melf_i386
AS = nasm
ASFLAGS = -f elf
MODULE_AS_FLAGS=-f bin

all: kernel.elf

run: iso
	./run_bochs.sh

iso: kernel.elf $(MODULES)
	./create_iso.sh $(MODULES)

kernel.elf: $(OBJECTS)
	ld $(LDFLAGS) $(OBJECTS) -o kernel.elf

.o:
	$(CC) $(CFLAGS) $< -o $@

.s.o:
	$(AS) $(ASFLAGS) $<

%.mod: %.s
	$(AS) $(MODULE_AS_FLAGS) $< -o $@

doc:
	cd .. ; doxygen doc/Doxyfile

clean:
	rm -rf *.o kernel.elf aenix.iso *.mod
